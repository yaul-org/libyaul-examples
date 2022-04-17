import os
import sys
import re
import math

ENTRIES = 64
WAY_COUNT = 4
CACHE_LINE = 16

entries = []

if sys.stdin.isatty():
    print(
        "sh2eb-elf-nm <file.elf> | python3 %s" % (os.path.basename(sys.argv[0])),
        file=sys.stderr,
        flush=True,
    )
    sys.exit(1)

for line in sys.stdin:
    line = line.strip()
    line_split = line.split(" ")
    if len(line_split) == 3:
        line_split = line_split[0:1] + ["0"] + line_split[1:]
    if len(line_split) != 4:
        continue
    address_str, size_str, symbol_type, symbol = line_split
    address = int(address_str, 16)
    address &= ~((1 << 31) | (1 << 30) | (1 << 29))
    size = int(size_str, 16)

    # THIS IS WRONG!!!!
    tag_mask = 0x1FFFFC00  # ((1 << 19) - 1) << 10
    entry_mask = 0x000003F0

    tag = address & tag_mask
    entry = (address & entry_mask) & (ENTRIES - 1)
    way = (tag >> 10) & (WAY_COUNT - 1)

    entry = {
        "address": address,
        "size": size,
        "symbol": symbol,
        "symbol_type": symbol_type,
        "tag": tag,
        "entry": entry,
        "way": way,
    }

    entries.append(entry)

for entry in entries:
    print(
        "%-40s<%s> 0x%08X: size: %8iB (%4i $Ls), tag: 0x%08X, entry: #%02i, way #%i"
        % (
            entry["symbol"],
            entry["symbol_type"],
            entry["address"],
            entry["size"],
            math.ceil(entry["size"] / float(CACHE_LINE)),
            entry["tag"],
            entry["entry"],
            entry["way"],
        )
    )
