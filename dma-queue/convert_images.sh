#!/usr/bin/env bash

trap 'exit_code=${?}; rm -f .out.tga; exit ${_exit_code}' 0 1

for tga in assets/*.TGA; do
    rm -f .out.tga
    convert "${tga}" -define tga:image-origin=BottomLeft -alpha off -compress RLE -flop .out.tga >/dev/null 2>&1 || { echo >&2 "Error converting ${tga}"; exit 1; }
    echo >&1 "Converting ${tga}"
    mv -v .out.tga "${tga}" >/dev/null 2>&1
done
