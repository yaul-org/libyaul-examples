import os
import sys
import struct

bit = int(sys.argv[3], 0)

with open(sys.argv[2], "wb+") as ofp:
    with open(sys.argv[1], "rb") as fp:
        while True:
            s = fp.read(2)
            if not s:
                break
            value, = struct.unpack('<H', s)
            ofp.write(struct.pack('>H', value | bit))
