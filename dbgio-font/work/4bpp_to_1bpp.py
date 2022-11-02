import os
import sys
import struct

if len(sys.argv) != 3:
  print("%s [input.cpd] [output-1bpp.cpd]" % (os.path.basename(sys.argv[0])))
  sys.exit(2)

input_cpd = sys.argv[1]
output_cpd = sys.argv[2]

with open(input_cpd, "rb") as ifp:
  data = []
  while ifp:
    data_read = ifp.read(4)
    if len(data_read) == 0:
        break
    data.append(struct.unpack("<L", data_read)[0])
  with open(output_cpd, "wb+") as fp:
    for packed in data:
      _1bpp = 0x00000000
      bit = 0
      while bit != 32:
        for byte_ in range(0, 4):
          unib = (packed & 0x0F) != 0x00
          packed >>= 4
          lnib = (packed & 0x0F) != 0x00
          packed >>= 4
          _1bpp |= (unib << (1 + bit)) | (lnib << bit)
          bit += 2
      bit = 0
      s = struct.pack("B", _1bpp)
      fp.write(s)
      _1bpp = 0x00000000
