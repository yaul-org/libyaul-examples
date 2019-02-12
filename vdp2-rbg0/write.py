import os
import struct
import sys

with open("romdisk/LEVEL1.MAP", "wb+") as fp:
    for x in range(0x2000 >> 1):
        p = struct.pack(">H", (54 << 1) + 0x200)
        fp.write(p)
