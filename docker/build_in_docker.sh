#!/bin/bash
set -eou pipefail

cd ..

pwd

rm -rf build
mkdir -p build

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -GNinja

cmake --build build -j"$(nproc)"

mkdir -p upload

cp "build/s3b" "upload"

cd "upload"
zip -r "s3b.zip" "build"