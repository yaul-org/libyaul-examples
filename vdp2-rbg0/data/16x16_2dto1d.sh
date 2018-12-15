#!/bin/bash

z=0

FILENAME="${1}"
WIDTH="${2}"
HEIGHT="${3}"

THEIGHT=$(((${HEIGHT} / 16) - 1))
TWIDTH=$(((${WIDTH} / 16) - 1))

for y in `seq 0 ${THEIGHT}`; do
    for x in `seq 0 ${TWIDTH}`; do
        out_z="tile${z}_16x16.png"
        convert -extract 16x16+$((x*16))+$((y*16)) "${FILENAME}" -size 16x16 +repage "${out_z}"
        ./2dto1d.sh "${out_z}" 16 16
        mv "out.png" "${out_z}"
        ((z++))
    done
done

convert -append `for n in \`seq 0 $((${z}-1))\`; do echo "tile${n}_16x16.png"; done | xargs` out.png

for f in `seq 0 $((${z} - 1))`; do
    rm -r -f "tile${f}_16x16.png"
done
