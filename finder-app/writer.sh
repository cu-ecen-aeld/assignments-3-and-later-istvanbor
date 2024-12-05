#!/bin/bash

# Check if the required arguments are provided
if [ "$#" -ne 2 ]; then
    echo "Error: Two arguments are required - file path and text string"
    exit 1
fi

# Extract arguments
writefile="$1"
writestr="$2"

# Check if writefile and writestr are specified
if [ -z "$writefile" ] || [ -z "$writestr" ]; then
    echo "Error: Both file path and text string must be specified"
    exit 1
fi

# Create the directory if it doesn't exist
mkdir -p "$(dirname "$writefile")"

# Write the content to the file
echo "$writestr" > "$writefile"

# Check if the file was created successfully
if [ "$?" -ne 0 ]; then
    echo "Error: Failed to create or write to the file"
    exit 1
fi

echo "File '$writefile' successfully created with content '$writestr'"
