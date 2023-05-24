#!/bin/bash

# Function to run the matching files
run_file() {
  local pattern="run_test_data.*\.bash"

  # Find all matching files and execute them
  find . -type f -regex "$pattern" -print0 | while IFS= read -r -d '' file; do
    echo "Running file: $file"
    # Add your desired command to execute the file here
    # For example: "./$file"
  done
}

# ANSI escape sequence for colors
GREEN='\033[0;32m'
RED='\033[0;31m'
RESET='\033[0m'

# Recursive function to traverse directories
traverse_directories() {
  local dir="$1"
  local file
  local subdirs

  # Traverse the current directory
  for file in "$dir"/*; do
    if [[ -d "$file" ]]; then
      # If the item is a directory, call the function recursively
      traverse_directories "$file"
    elif [[ -f "$file" ]]; then
      # If the item is a file, check if it matches the pattern
      if [[ $file =~ .*run_test_data.*\.bash$ ]]; then
        current_dir=$(pwd)
        directory=$(dirname "$file")
        filename=$(basename "$file")
        cd "$directory"
        echo "-----------------------------------------"
        echo "Running tests: starting $file"
        chmod a+x "$filename"
        result=$(bash "$filename")
        exit_code=$?
        if [ $exit_code == 0 ]; then
          echo -e "${GREEN}===== SUCCESS =====${RESET}"
        else
          echo -e "${RED}===== FAIL =====${RESET}"
          echo "Exit code = $exit_code"
          echo "Run result: $result"
        fi
        cd "$current_dir"
      fi
    fi
  done
}

# Start the traversal from the current directory
traverse_directories "."
