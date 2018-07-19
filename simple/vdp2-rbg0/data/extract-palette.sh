#!/bin/bash

file="${1}"
out_file="${2}"
color_count="${3}"

convert "${file}" -format "%c" -depth 8  histogram:info:- | awk '{
  sub(/^\s+[0-9]+:\s+/, "");
  sub(/ \#[0-9a-zA-Z]+.+$/, "");
  gsub(/ /, "");
  print "rgb" $0
}' > "palette.lst"

palette_count=`cat "palette.lst" | wc -l`

# Round up to the nearest ${color_count}
padded_count=$((((${palette_count} + ${color_count} - 1) / ${color_count}) * ${color_count}))
rem_count=$((${padded_count} - ${palette_count}))

echo "${rem_count}"

for x in `seq ${rem_count}`; do
    echo "rgb(0,0,0)" >> "palette.lst"
done

convert \
-size 8x8\! \
`cat "palette.lst" | while read -r line; do
  echo "xc:${line}"
done` \
+append \
"${out_file}"
