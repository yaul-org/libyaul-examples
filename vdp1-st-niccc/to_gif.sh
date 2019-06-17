#!/bin/bash -e

# https://medium.com/@colten_jackson/doing-the-gif-thing-on-debian-82b9760a8483

time=${1}
in_file=${2}
out_file=${3}

# game="${1}
# make clean; and make; and mednafen -qtrecord out.mov ${game}.cue
# ../vdp2-bitmap/to_gif.sh 11.4326 out.mov out.gif
# mkdir .images
# cp ../vdp2-bitmap/README.markdown .
# mv out.gif .images/preview.gif
# git add .images/preview.gif README.markdown
# git commit -m "Add preview to ${game}"

# 1. make clean; and make; and mednafen -qtrecord out.mov example.cue
# 2. ffplay -vf showinfo -ss X out.mov          # Find out where the SEGA screen ends
# 3. Use this script

# ffmpeg -y -ss ${time} -i ${in_file} -vf "fps=15,scale=320:224:flags=lanczos,palettegen" palette.png
# ffmpeg -y -ss ${time} -i ${in_file} -i palette.png -filter_complex "fps=15,scale=320:224:flags=lanczos[x];[x][1:v]paletteuse" ${out_file}

ffmpeg -y -ss ${time} -i ${in_file} \
       -filter_complex "[0:v] fps=15,scale=w=320:h=224,split [a][b];[a] palettegen [p];[b][p] paletteuse" \
       ${out_file} >ffmpeg.log 2>&1

du -hs ${out_file}
