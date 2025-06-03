#!/bin/bash

# Check if directory argument is provided
if [ $# -eq 0 ]; then
    echo "Error: Please provide a directory as an argument"
    exit 1
fi

target_dir="$1"

# Check if directory exists
if [ ! -d "$target_dir" ]; then
    echo "Error: Directory '$target_dir' does not exist"
    exit 1
fi

# Process each file in the directory
for file in "$target_dir"/*; do
    if [ -f "$file" ] && [ ! -d "$file" ]; then  # Check if it's a regular file
        # Get the base name
        base_name=$(basename "$file")
        
        echo "Splitting $file..."
        
        # Split the file and only delete original if split succeeds
        if split -d -C 2G "$file" "$target_dir/$base_name."; then
            rm "$file" && echo "Removed original file: $file"
        else
            echo "Error: Failed to split $file (original file kept)"
            exit 1
        fi
    fi
done

echo "All files processed."
