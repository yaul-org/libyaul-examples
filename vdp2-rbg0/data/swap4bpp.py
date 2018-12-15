import os
import sys
import struct

with open(sys.argv[2], "wb+") as ofp:
    with open(sys.argv[1], "rb") as fp:
        while True:
            byte = fp.read(1)
            if not byte:
                break
            value, = struct.unpack('B', byte)
            out_value = ((value >> 4) | ((value & 0x0F) << 4))
            ofp.write(struct.pack('B', out_value))
