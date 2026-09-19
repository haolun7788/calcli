Simple C++ CLI for quick Google Calender entries. Work in Progress.

./build/Debug/calcli

cmake --build build
ctest --test-dir build --output-on-failure

cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=~/vcpkg/scripts/buildsystems/vcpkg.cmake