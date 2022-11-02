import os
import sys
import struct

if len(sys.argv) != 3:
    print("%s [input.map] [output.pnd]" % (os.path.basename(sys.argv[0])))
    sys.exit(2)

input_map = sys.argv[1]
output_pnd = sys.argv[2]

def extract_map(filename, out_name):
    with open(filename, "rb") as fp:
        fp.read(8)
        with open(out_name, "wb+") as out:
            while True:
                data = fp.read(2)
                if not data:
                    break
                short, = struct.unpack(">H", data)
                short += (0x4000 >> 5) >> 2
                out.write(struct.pack(">H", short))

extract_map(input_map, output_pnd)

# extract_map("nbg0.map", "../assets/NBG0.PND")
# extract_map("ngb1.map", "../assets/NBG1.PND")
# extract_map("ngb2.map", "../assets/NBG2.PND")
# extract_map("nbg3.map", "../assets/NBG3.PND")
