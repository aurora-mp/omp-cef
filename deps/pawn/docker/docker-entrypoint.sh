#!/bin/sh
[ -z $CONFIG ] && config=Release || config="$CONFIG"

cmake --version

cmake \
    -S ./source/compiler \
    -B build \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=$config \
    -DCMAKE_C_FLAGS="-m32 -DsNAMEMAX=63" \
    -DBUILD_TESTING=False \
&&
cmake \
    --build build \
    --config $config \
    --parallel $(nproc)
