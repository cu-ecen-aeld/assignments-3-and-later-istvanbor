#!/bin/sh

# Clean previous build artifacts
echo "Cleaning previous build artifacts..."
make clean

# Compile the writer application using native compilation
echo "Compiling writer application..."
make

# Check if the required arguments are provided
if [ "$#" -ne 2 ]; then
    echo "Error: Two arguments are required - directory path and search string"
    exit 1
fi

# Extract arguments
filesdir="$1"
searchstr="$2"

# Check if filesdir exists and is a directory
if [ ! -d "$filesdir" ]; then
    echo "Error: The specified directory does not exist or is not a directory"
    exit 1
fi

# Find matching lines in files
matching_lines=$(grep -r "$searchstr" "$filesdir" 2>/dev/null)

# Get the number of files and matching lines
num_files=$(find "$filesdir" -type f | wc -l)
num_matching_lines=$(echo "$matching_lines" | wc -l)

# Print the results
echo "The number of files are $num_files and the number of matching lines are $num_matching_lines"

# Write the results using the writer utility
output_file="/tmp/finder-output.txt"
echo "Writing results to $output_file using writer utility..."
./writer "$output_file" "The number of files are $num_files and the number of matching lines are $num_matching_lines"

if [ "$?" -ne 0 ]; then
    echo "Error: Failed to write results to $output_file"
    exit 1
fi

echo "Results successfully written to $output_file"

