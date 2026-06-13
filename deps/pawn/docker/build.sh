#!/bin/bash

# Available configs: Debug, [RelWithDebInfo], Release, MinSizeRel
[[ -z "$CONFIG" ]] \
&& config=RelWithDebInfo \
|| config="$CONFIG"

docker build \
    -t omp-pawn-compiler/build:ubuntu-18.04 ./ \
|| exit 1

folders=('build')
for folder in "${folders[@]}"; do
    if [[ ! -d "./${folder}" ]]; then
        mkdir ${folder}
    fi
    sudo chown -R 1000:1000 ${folder} || exit 1
done

docker run \
    --rm \
    -t \
    -w /code \
    -v $PWD/..:/code \
    -v $PWD/build:/code/build \
    -e CONFIG=${config} \
    omp-pawn-compiler/build:ubuntu-18.04
