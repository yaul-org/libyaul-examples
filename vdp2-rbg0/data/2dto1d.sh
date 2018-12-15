#!/bin/bash

FILENAME="${1}"
WIDTH="${2}"
HEIGHT="${3}"

TWIDTH=$(((${WIDTH} / 8) - 1))
THEIGHT=$(((${HEIGHT} / 8) - 1))

z=0
for y in `seq 0 ${THEIGHT}`; do
    for x in `seq 0 ${TWIDTH}`; do
        convert -extract 8x8+$((x*8))+$((y*8)) "${FILENAME}" +repage -size 8x8 "tile${z}.png"
        
        ((z++))
    done
done

convert -append `for n in \`seq 0 $((${z}-1))\`; do echo "tile${n}.png"; done | xargs` out.png

for f in `seq 0 $((${z} - 1))`; do
    rm -r -f "tile${f}.png"
done
