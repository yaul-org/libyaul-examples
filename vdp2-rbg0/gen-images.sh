#!/bin/bash -e

cd data

rm -f *.bin
rm -f ../romdisk/*.CEL
rm -f ../romdisk/*.PAL

# How to merge palettes from multiple tilesheets together:
#
# 1. All the tilesheets either share or combined, must not exceed 16,
#    256, or 2048 colors.
#
# 2. Merge all the images together either via Gimp or ImageMagick
#    (+append). Or dedicate a tile (16x16 or 8x8) and store the
#    palette there.
#
# 3. Use extract-palette.sh to create a shared-palette.png image.
#
# 4. Import shared-palette.png into Gimp.
#
# 5. Convert all individual tilesheets from indexed to RGB if they're
#    not already in RGB.
#
# 6. Convert to indexed and use the palette from the
#    shared-palette.png.
#
# 7. Save and export.
#
# 8. Export the shared palette as a .PAL.

printf -- "Processing palette\n"
../grit/grit shared-palette.png -g\! -p -pn 256 -fh\! -ftb
python2.7 wordswap.py shared-palette.pal.bin shared-palette.pal.bin.swp 0x0000
mv shared-palette.pal.bin.swp ../romdisk/TILESET.PAL

printf -- "Processing tileset2_1d.png\n"
../grit/grit.exe tileset2_1d.png -g -gt -gB8 -m\! -p\! -fh\! -ftb
mv tileset2_1d.img.bin ../romdisk/TILESET.CEL

printf -- "Processing tileset_mc.png\n"
../grit/grit.exe tileset_mc.png -g -gb -gB8 -m\! -p\! -fh\! -ftb
mv tileset_mc.img.bin ../romdisk/TSMC.CEL

printf -- "Processing tileset_menu.png\n"
../grit/grit.exe tileset_menu.png -g -gb -gB8 -m\! -p\! -fh\! -ftb
mv tileset_menu.img.bin ../romdisk/TSMENU.CEL

rm -f *.bin
