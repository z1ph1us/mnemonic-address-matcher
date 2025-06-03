#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <unordered_set>
#include <atomic>
#include <mutex>
#include <thread>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <immintrin.h>
#include <nlohmann/json.hpp> 

namespace fs = std::filesystem;
using json = nlohmann::json;

// --- Constants ---
constexpr size_t CACHE_LINE_SIZE = 64;
constexpr size_t L3_CACHE_SIZE = 24 * 1024 * 1024;
constexpr size_t PARALLEL_CHUNKS = 12;

// --- Struct for memory-mapped file ---
struct MappedFile {
    int fd = -1;
    size_t size = 0;
    const char* data = nullptr;

    bool open(const std::string& path) {
        fd = ::open(path.c_str(), O_RDONLY);
        if (fd < 0) return false;
        size = lseek(fd, 0, SEEK_END);
        data = (const char*)mmap(nullptr, size, PROT_READ, MAP_PRIVATE | MAP_POPULATE, fd, 0);
        return data != MAP_FAILED;
    }

    void close() {
        if (data) munmap((void*)data, size);
        if (fd >= 0) ::close(fd);
        data = nullptr;
        fd = -1;
        size = 0;
    }

    ~MappedFile() {
        close();
    }
};

// --- Utility: List files in folder or single file ---
std::vector<fs::path> getFiles(const std::string& path) {
    std::vector<fs::path> files;
    if (fs::is_directory(path)) {
        for (auto& entry : fs::directory_iterator(path)) {
            if (entry.is_regular_file()) {
                files.push_back(entry.path());
            }
        }
        std::sort(files.begin(), files.end());
    } else if (fs::is_regular_file(path)) {
        files.push_back(path);
    } else {
        throw std::runtime_error("Path is neither file nor directory: " + path);
    }
    return files;
}

// --- Improved AddressIndex class ---
class AddressIndex {
    std::vector<const char*> addresses;
    std::vector<size_t> lengths;
    std::unordered_set<std::string> address_set; // For faster lookups

public:
    bool build(const MappedFile& file) {
        if (!file.data || file.size == 0) return false;
        size_t line_count = std::count(file.data, file.data + file.size, '\n');
        addresses.reserve(line_count);
        lengths.reserve(line_count);
        address_set.reserve(line_count);

        const char* ptr = file.data;
        const char* end = file.data + file.size;

        while (ptr < end) {
            const char* line_end = static_cast<const char*>(memchr(ptr, '\n', end - ptr));
            if (!line_end) line_end = end;

            // Each line in funded file is just an address
            size_t len = line_end - ptr;
            if (len > 0) {
                addresses.push_back(ptr);
                lengths.push_back(len);
                address_set.emplace(ptr, len);
            }

            ptr = line_end + 1;
        }

        return true;
    }

    bool contains(const std::string& addr) const {
        return address_set.find(addr) != address_set.end();
    }
};

// --- Improved process one input chunk against one funded chunk ---
void process_chunk(const MappedFile& input, const AddressIndex& findex,
                   size_t start, size_t end, std::atomic<size_t>& matches,
                   std::mutex& output_mutex, std::ofstream& output_file) {
    constexpr size_t BLOCK_SIZE = L3_CACHE_SIZE / PARALLEL_CHUNKS;
    std::vector<std::string> local_matches;
    local_matches.reserve(100000);

    for (size_t pos = start; pos < end; pos += BLOCK_SIZE) {
        size_t block_end = std::min(pos + BLOCK_SIZE, end);
        const char* ptr = input.data + pos;
        const char* block_end_ptr = input.data + block_end;

        while (ptr < block_end_ptr) {
            const char* line_end = static_cast<const char*>(memchr(ptr, '\n', block_end_ptr - ptr));
            if (!line_end) break;

            // Each line is in format: "phrase/seed phrase tab address"
            const char* tab_pos = static_cast<const char*>(memchr(ptr, '\t', line_end - ptr));
            if (tab_pos) {
                // Extract the address part (after the tab)
                const char* addr_start = tab_pos + 1;
                size_t addr_len = line_end - addr_start;
                
                if (addr_len > 0) {
                    std::string address(addr_start, addr_len);
                    if (findex.contains(address)) {
                        local_matches.emplace_back(ptr, line_end);
                        matches.fetch_add(1, std::memory_order_relaxed);
                    }
                }
            }

            ptr = line_end + 1;
        }
    }

    if (!local_matches.empty()) {
        std::lock_guard<std::mutex> lock(output_mutex);
        for (const auto& m : local_matches) {
            output_file << m << '\n';
        }
        output_file.flush();
    }
}

int main(int argc, char** argv) {
    if (argc != 7) {
        std::cerr << "Usage: " << argv[0] << " -f <funded_path> -i <input_path> -o <output_file>\n";
        return 1;
    }


    std::cout << "Crypto Mnemonic-Address Matcher - Made by z1ph1us\n";
    std::cout << "---------------------------------------------------\n";

    std::string funded_path, input_path, output_path;
    for (int i = 1; i < argc; i += 2) {
        std::string flag = argv[i];
        if (flag == "-f") funded_path = argv[i + 1];
        else if (flag == "-i") input_path = argv[i + 1];
        else if (flag == "-o") output_path = argv[i + 1];
        else {
            std::cerr << "Unknown flag: " << flag << "\n";
            return 1;
        }
    }

    try {
        auto funded_files = getFiles(funded_path);
        auto input_files = getFiles(input_path);

        if (funded_files.empty()) {
            std::cerr << "No funded files found.\n";
            return 1;
        }
        if (input_files.empty()) {
            std::cerr << "No input files found.\n";
            return 1;
        }

        // Open output file in append mode
        std::ofstream output_file(output_path, std::ios::out | std::ios::app);
        if (!output_file) {
            std::cerr << "Failed to open output file: " << output_path << "\n";
            return 1;
        }

        std::atomic<size_t> total_matches{0};
        std::mutex output_mutex;

        auto start_time = std::chrono::steady_clock::now();

        for (const auto& funded_file : funded_files) {
            std::string funded_name = funded_file.filename().string();

            MappedFile funded_chunk;
            if (!funded_chunk.open(funded_file.string())) {
                std::cerr << "Warning: Failed to open funded chunk: " << funded_file << std::endl;
                continue;
            }

            AddressIndex findex;
            if (!findex.build(funded_chunk)) {
                std::cerr << "Warning: Failed to build index for funded chunk: " << funded_file << std::endl;
                funded_chunk.close();
                continue;
            }

            for (const auto& input_file_path : input_files) {
                std::string input_name = input_file_path.filename().string();

                MappedFile input_file;
                if (!input_file.open(input_file_path.string())) {
                    std::cerr << "Warning: Failed to open input chunk: " << input_file_path << std::endl;
                    continue;
                }

                std::cout << "Processing funded chunk '" << funded_name << "' with input chunk '" << input_name << "'\n";

                // --- Patch for small files ---
// If the input file is small, process it in a single chunk/thread
if (input_file.size < 1024 * 1024 || input_file.size < PARALLEL_CHUNKS * 128) {
    process_chunk(input_file, findex, 0, input_file.size, total_matches, output_mutex, output_file);
} else {
    // Large file: process in parallel as before
    std::vector<std::thread> workers;
    const size_t chunk_size = (input_file.size + PARALLEL_CHUNKS - 1) / PARALLEL_CHUNKS;
    const size_t aligned_chunk = (chunk_size + CACHE_LINE_SIZE - 1) & ~(CACHE_LINE_SIZE - 1);

    for (unsigned t = 0; t < PARALLEL_CHUNKS; ++t) {
        size_t start = t * aligned_chunk;
        size_t end = std::min(start + aligned_chunk, input_file.size);

        if (start > 0) {
            while (start < input_file.size && input_file.data[start - 1] != '\n') ++start;
        }

        workers.emplace_back(process_chunk, std::ref(input_file), std::ref(findex),
                             start, end, std::ref(total_matches),
                             std::ref(output_mutex), std::ref(output_file));
    }

    for (auto& w : workers) {
        w.join();
    }
}


                input_file.close();
            }

            funded_chunk.close();

            auto elapsed = std::chrono::steady_clock::now() - start_time;
            std::cout << "Finished funded chunk '" << funded_name << "'. Total matches so far: "
                      << total_matches.load() << ". Elapsed time: "
                      << std::chrono::duration_cast<std::chrono::seconds>(elapsed).count()
                      << " seconds.\n\n";
        }

        output_file.close();

        auto total_elapsed = std::chrono::steady_clock::now() - start_time;
        std::cout << "All processing done. Total matches: " << total_matches.load()
                  << ". Total elapsed time: "
                  << std::chrono::duration_cast<std::chrono::seconds>(total_elapsed).count()
                  << " seconds.\n";

    } catch (const std::exception& ex) {
        std::cerr << "Fatal error: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
