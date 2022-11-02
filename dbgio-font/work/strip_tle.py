import sys
import os
import sys
import struct

if len(sys.argv) != 4:
  print("%s [input.tle] [output.cpd] [output.pal]" % (os.path.basename(sys.argv[0])))
  sys.exit(2)

input_tle = sys.argv[1]
output_cpd = sys.argv[2]
output_pal = sys.argv[3]

with open(input_tle, "rb") as ifp:
  ifp.read(4) # Skip palette length
  ifp.read(4) # Skip palette type
  palette = ifp.read(32)
  ifp.read(4) # Skip data size
  with open(output_cpd, "wb+") as ofp:
    ofp.write(ifp.read())
  with open(output_pal, "wb+") as ofp:
    ofp.write(palette)
