import os
import struct
import sys

if len(sys.argv) != 4:
  print("%s [input.tle] [output.cpd] [output.pal]" % (os.path.basename(sys.argv[0])))
  sys.exit(2)

input_tle = sys.argv[1]
output_cpd = sys.argv[2]
output_pal = sys.argv[3]

with open(input_tle, "rb") as fp:
  pal_count, = struct.unpack(">L", fp.read(4))
  pal_type, = struct.unpack(">L", fp.read(4))
  palette = fp.read(pal_count * 2)
  with open(output_pal, "wb+") as out:
    out.write(palette)
  image_size, = struct.unpack(">L", fp.read(4))
  image = fp.read(image_size)
  with open(output_cpd, "wb+") as out:
    out.write(image)
