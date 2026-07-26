#!/bin/bash
set -eou pipefail

echo "CMake Version: $(cmake --version)"

# rm -rf dockerbuild
echo "creating build directory..."
mkdir -pv /cache/dockerbuild

echo "configuring project..."
cmake -S . -B /cache/dockerbuild -DCMAKE_BUILD_TYPE=RelWithDebInfo -GNinja -DBUILD_FOR_DIST=ON

echo "building project..."
cmake --build /cache/dockerbuild --target s3b -j"$(nproc)"

echo "creating upload dir..."
mkdir -pv upload


cp -v "bin/s3b" "upload"

cd "upload"

echo "creating release zip..."
zip -r "s3b.zip" "s3b"