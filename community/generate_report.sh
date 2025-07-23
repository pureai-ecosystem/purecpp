#!/bin/bash

OUTPUT="report.txt"
echo "Generating system report into $OUTPUT..."

{
    echo -e "What happen?\n"

    echo "=== System Report ==="
    echo "OS and Kernel:"
    uname -a

    echo -e "\nCPU Info:"
    lscpu | grep -E 'Model name|Socket|Thread|Core'

    echo -e "\nMemory Info:"
    free -h

    echo -e "\nGCC Version:"
    gcc --version | head -n1

    echo -e "\nCMake Version:"
    cmake --version | head -n1

    echo -e "\nConan Version:"
    conan --version | head -n1

    echo -e "\nPython Version:"
    python3 --version

    echo -e "\nPython Package Check (build, conan, cmake, requests, pybind11):"
    for pkg in build conan cmake requests pybind11; do
        python3 -m pip show $pkg 2>/dev/null | grep Version || echo "$pkg: Not installed"
    done

    echo -e "\nEnvironment Variables (limited view):"
    env | grep -E 'CMAKE|OMP|LD_LIBRARY_PATH'



} > "$OUTPUT" 2>&1

echo "Report saved to $OUTPUT"
