#!/bin/sh
set -e
cmake -B build -DCMAKE_BUILD_TYPE=Release "$@"
cmake --build build
echo "=== Build complete ==="
echo ""
printf "Run unit tests? (Y/N): "
read -r answer
if [ "$answer" = "Y" ] || [ "$answer" = "y" ]; then
    cd build && ctest --output-on-failure
    cd ..
    echo ""
    printf "Run end-to-end test? (Y/N): "
    read -r e2e
    if [ "$e2e" = "Y" ] || [ "$e2e" = "y" ]; then
        tests/scripts/test_e2e.sh
    fi
fi
echo ""
rm -rf build
echo "=== Done (build cleaned) ==="
