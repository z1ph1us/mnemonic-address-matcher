---
## Description

This high-performance tool matches cryptocurrency addresses between two datasets:

- Your **mnemonic phrase list** (containing seed phrases and their associated addresses)
- A **target address list** (containing addresses of interest)

When a match is found, the tool outputs:

- The **matching cryptocurrency address**
- Its corresponding **mnemonic phrase/seed phrase**

---

### Key Features

* **Mnemonic Discovery:** Quickly identifies mnemonic phrases associated with addresses present in your target list.
* **Optimized Performance:** Achieves high processing speeds through the intelligent use of multi-threading and memory-mapped files, built for handling extensive datasets.
* **Scalable File Handling:** Capable of processing extremely large files by breaking them into smaller, manageable chunks, which helps in avoiding memory exhaustion.
* **Flexible Input:** Supports both individual files and directories containing multiple split files, offering adaptability for various data organization methods.

---

### Installation

#### What You Need

Before proceeding with the installation, ensure your system meets these requirements:

* **Linux/macOS:** The tool is developed for Unix-like operating systems. For Windows users, Windows Subsystem for Linux (WSL) or Cygwin is recommended.
* **Modern C++ Compiler:** A C++ compiler such as GCC (version 9 or newer) or Clang (version 10 or newer) is required.
* **`nlohmann/json` library:** This is a header-only C++ library for JSON parsing and generation.


---

#### Easy Setup with `install.sh` (For Debian/Ubuntu Linux)

The easiest way to set up the tool on **Debian or Ubuntu-based Linux systems** is by using the included `install.sh` script. This script will:

1.  Update your system's software lists.
2.  Install all required tools and libraries (like `g++`, `cmake`, `git`, and `nlohmann-json3-dev`).
3.  Compile the `address_matcher` program.
4.  Move the finished program to the main project folder, getting it ready for you to use.

To run it:

```bash
# Make the script runnable (you only need to do this once)
chmod +x install.sh

# Run the installation script
./install.sh
```

The script will tell you what's happening. If it can't install something, it'll warn you so you can install it yourself if needed.

---

### Usage

#### Prepare Your Files

For optimal performance and correct operation, your input files must adhere to specific formats:

* **Input Files (Mnemonic & Address):** Each line in these files should contain a **mnemonic phrase**, followed by a **tab character**, and then the corresponding **cryptocurrency address**.

    ```
    mnemonic phrase here    11111111111111111111CJawggc
    another phrase here    111111111111111111112xT3273
    ```

* **Target Address Files:** These files should simply list **one cryptocurrency address per line**. These are the addresses you wish to find matches for.

    ```
    1111111111111111111114oLvT2
    111111111111111111112BEH2ro
    ```

#### Splitting Large Files

If you are working with very large files (multiple gigabytes), it is highly recommended to split them into smaller parts. This improves processing efficiency and helps prevent memory issues. Use the `split.sh` script for this purpose:

```bash
# Make the splitting script executable
chmod +x split.sh

# Split your target address files located in the 'Funded/' directory
./split.sh Funded/
# or for a single file:
./split.sh path/to/your/file.txt

# Split your mnemonic-address files located in the 'Input/' directory
./split.sh Input/
# or for a single file:
./split.sh path/to/your/file.txt
```

**IMPORTANT NOTE:** When `split.sh` processes a file, it will divide the original files located in a folder smaller chunks and then **delete the original, large file**. The original file will no longer exist in its single, large form; it will be replaced by the newly created smaller chunks.

The script will divide your files into 2GB chunks and append sequential numeric suffixes (e.g., `original_file.txt.00`, `original_file.txt.01`).

#### Running the Matcher

Once your files are prepared and, if necessary, split, execute the `address_matcher` using the following command:

```bash
./address_matcher -f Funded/ -i Input/ -o matches.txt
```

**Command-line Arguments:**

* `-f <path>`: Specifies the path to your **target address file** or the directory containing them.
* `-i <path>`: Specifies the path to your **mnemonic-address input files** or the directory containing them.
* `-o <file>`: Specifies the **output file** where any identified matches will be written.

#### Example Workflow

Here is a complete example demonstrating the typical workflow:

1.  **Organize your data files:**

    ```bash
    mkdir -p Funded Input
    # Place your list of target addresses into files within the 'Funded/' directory (e.g., Funded/addresses.txt)
    # Place your mnemonic-address pairs into files within the 'Input/' directory (e.g., Input/mnemonic_pairs.txt)
    ```

2.  **Split any large files for optimized processing (if applicable):**

    ```bash
    ./split.sh Funded/
    ./split.sh Input/
    ```

3.  **Execute the matching tool:**

    ```bash
    ./address_matcher -f Funded/ -i Input/ -o found_mnemonics.txt
    ```

4.  **Review the results:**

    ```bash
    cat found_mnemonics.txt
    ```

---
*You can easily test the tool using the small `Funded/` and `Input/` example folders with files presented in the repository. They're here for a quick test run and will output 2 matching results.*
---


### Performance Optimization Tips

To achieve the highest possible performance from the `Crypto Mnemonic-Address Matcher`, consider these recommendations:

* **Split All Files:** For maximum efficiency, ensure both your **target address files** and your **mnemonic-address input files** are split into chunks. This enables more effective parallel processing.
* **Utilize SSD Storage:** Storing your data files on **Solid State Drives (SSDs)** will significantly reduce file input/output times, which is critical for large datasets.
* **Allocate Sufficient RAM:** The tool benefits greatly from available memory. It is recommended to allocate at least **4GB of RAM per parallel chunk** used for processing.

