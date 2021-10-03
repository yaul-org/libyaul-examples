#!/bin/bash

# convert giphy.gif -coalesce -type truecolor -gravity northwest -extent 512x256 -type truecolor %04d.bmp

i=0
for x in *.bmp; do
    grit "${x}" -g -gb -gB16 -m\! -ftb -fh\! -oout
    printf -- "${x}: "
    bcl_prs out.img.bin cd/$(printf "Z%03i" ${i}).BIN
    rm -f out.img.bin
    i=$((${i} + 1))
done
