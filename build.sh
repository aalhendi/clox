#!/bin/bash

# Create build directory if it doesn't exist
mkdir -p build

# Change to build directory
cd build

# Run CMake to generate build files
cmake ..

# Build the project
cmake --build .

# Return to the original directory
cd -

