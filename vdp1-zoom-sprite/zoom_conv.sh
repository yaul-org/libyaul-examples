#!/bin/bash

# This strips unneeded info and prevents the files from being seen as
# modified by git
PNG_OPTS="-define png:include-chunk=none -strip"

# Extract from zoom.png into 64x128 PNG images, then append them all
# vertically into zoom_appended.png
cd data
rm -f zoom_[0-9][0-9].png
rm -f zoom_appended.png
for x in `seq 0 13`; do
    zoom=`echo "$x*75" | bc`
    in=`printf "zoom_%02i.png" "$x"`
    convert -extract 64x128+"${zoom}"+0 "zoom.png" +repage -size 64x128 ${PNG_OPTS} "${in}"
done
convert -append zoom_*.png ${PNG_OPTS} zoom_appended.png
cd ..

# Convert
grit "data/zoom_appended.png" -g -gb -gB8 -m\! -p -pT1 -fh\! -ftb

# Move to assets/
mv "zoom_appended.pal.bin" "assets/ZOOM.PAL"
mv "zoom_appended.img.bin" "assets/ZOOM.TEX"
