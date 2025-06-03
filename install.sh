#!/bin/bash

echo "Starting the installation process for Crypto Mnemonic-Address Matcher..."
echo "-----------------------------------------------------"

# --- 1. Check for sudo ---
if ! command -v sudo &> /dev/null
then
    echo "Error: 'sudo' command not found."
    echo "Please install sudo or run this script as a root user."
    exit 1
fi

# --- 2. Update package lists ---
echo "Updating system package lists..."
if sudo apt update; then
    echo "Package lists updated successfully."
else
    echo "Warning: Failed to update package lists."
    echo "Please try running 'sudo apt update' manually if you encounter further issues."
    sleep 2 # Pause to allow user to read the warning
fi

# --- 3. Install required packages ---
# List of essential packages for compilation and functionality
PACKAGES=(
    "g++"                 # C++ compiler
    "cmake"               # Build system (useful, though direct g++ is used here)
    "git"                 # Version control (for cloning, if not already cloned)
    "nlohmann-json3-dev"  # Header-only JSON library development files
)

echo "Attempting to install required packages..."
for pkg in "${PACKAGES[@]}"; do
    echo "   - Installing $pkg..."
    if sudo apt install -y "$pkg"; then
        echo "     $pkg installed successfully."
    else
        echo "     Warning: Failed to install $pkg."
        echo "     Please install it manually using 'sudo apt install $pkg' if compilation fails."
        sleep 2 # Pause for user to read warning
    fi
done

echo "-----------------------------------------------------"

# --- 4. Compile the C++ application ---
# Check if the source file exists before attempting to compile
if [ -f "src/address_matcher.cpp" ]; then
    echo "Compiling address_matcher from src/address_matcher.cpp..."
    # Compilation command as specified in the README
    # -std=c++17: Enables C++17 features
    # -O3: Aggressive optimization for performance
    # -march=native: Optimizes for your specific CPU architecture
    # -pthread: Links against the POSIX threads library for multi-threading
    # -o src/address_matcher: Specifies the output executable name within the src/ directory
    # -lstdc++fs: Links against the C++ filesystem library
    g++ -std=c++17 -O3 -march=native -pthread -o src/address_matcher src/address_matcher.cpp -lstdc++fs

    # Check if compilation was successful
    if [ $? -eq 0 ]; then
        echo "Compilation successful!"
        echo "-----------------------------------------------------"

        # --- 5. Move the compiled executable to the root directory ---
        echo "Moving compiled executable to the root directory..."
        if mv src/address_matcher .; then
            echo "'address_matcher' moved successfully to the project root."
            echo "-----------------------------------------------------"
            echo "Installation complete!"
            echo "You can now run the tool using: ./address_matcher"
        else
            echo "Error: Failed to move 'address_matcher' executable."
            echo "It might still be located in the 'src/' directory."
            exit 1
        fi
    else
        echo "Error: Compilation failed."
        echo "Please review the output above for specific error messages and ensure all prerequisites are met."
        exit 1
    fi
else
    echo "Error: Source file 'src/address_matcher.cpp' not found."
    echo "Please ensure you are running this script from the project's root directory"
    echo "and that the 'src/address_matcher.cpp' file exists."
    exit 1
fi

echo "-----------------------------------------------------"
