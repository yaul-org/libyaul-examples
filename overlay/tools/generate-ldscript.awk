#!/usr/bin/awk -f

BEGIN {
    FS = ",";
    L = 0;

    delete dirs_arr[0];
    delete symbols_arr[0];
    count = 0;
}

# Strip leading and trailing whitespace
{ gsub(/^\s*/, ""); gsub(/^\s*$/, ""); L++; }

# Remove empty dirs_arr
/^$/ { next; }

# Remove leading comments
/^#.*$/ { next; }

# Remove trailing comments
/#.*$/ { gsub(/\s*#.*$/, ""); }

{
    if (/^\s*.*\s*,\s*[_a-zA-Z][_a-zA-Z0-9]{0,30}\s*$/) {
        gsub(/^\s*/, "", $1);
        gsub(/\s*$/, "", $1);
        gsub(/^\s*/, "", $2);
        gsub(/\s*$/, "", $2);
        dir = $1;
        symbol = $2;

        dirs_arr[count] = dir;
        symbols_arr[count] = symbol;
        count++;
    } else {
        printf "generate-ldscript: Syntax error: L%i: \"%s\"\n", L, $0;
        exit 1;
    }
}

END {
    for (i = 0; i < count; i++) {
        dir = dirs_arr[i];
        symbol = symbols_arr[i];

        printf "\n";
        printf "   .%s.uncached (0x20000000 | __%s_end) : AT (__%s_load_end)\n", symbol, symbol, symbol;
        printf "   {\n";
        printf "      . = ALIGN (4);\n";
        printf "      \"%s\"(.uncached .uncached.*)\n", dir;
        printf "      . = ALIGN (4);\n";
        printf "   }\n";
        printf "   __%s_uncached_start = LOADADDR (.%s.uncached);\n", symbol, symbol;
        printf "   __%s_uncached_end = LOADADDR (.%s.uncached) + SIZEOF (.%s.uncached);\n", symbol, symbol, symbol;
        printf "   __%s_uncached_size = SIZEOF (.%s.uncached);\n", symbol, symbol;
    }

    printf "\n";
    printf "   __overlay_load_offset = __lma_end;\n";

    for (i = 0; i < count; i++) {
        dir = dirs_arr[i];
        symbol = symbols_arr[i];

        printf "\n";
        printf "   .%s ___overlay_start : AT (__overlay_load_offset)\n", symbol;
        printf "   {\n";
        printf "      . = ALIGN (4);\n";
        printf "\n";
        printf "      KEEP (\"*%s*\"(.text.%s))\n", dir, symbol;
        printf "      \"*%s*\"(.text .text.* .gnu.linkonce.t.*)\n", dir;
        printf "      \"*%s*\"(.rdata .rodata .rodata.* .gnu.linkonce.r.*)\n", dir;
        printf "      \"*%s*\"(.data .data.* .gnu.linkonce.d.* .sdata .sdata.* .gnu.linkonce.s.*)\n", dir;
        printf "      \"*%s*\"(.bss .bss.* .gnu.linkonce.b.* .sbss .sbss.* .gnu.linkonce.sb.* .scommon COMMON)\n", dir;
        printf "\n";
        printf "      *(.%s.uncached)\n", symbol;
        printf "\n";
        printf "      . = ALIGN (4);\n";
        printf "   }\n";
        printf "   __%s_start = ADDR (.%s);\n", symbol, symbol;
        printf "   __%s_end = ADDR (.%s) + SIZEOF (.%s);\n", symbol, symbol, symbol;
        printf "   __%s_size = SIZEOF (.%s) + __%s_uncached_size;\n", symbol, symbol, symbol;
        printf "   __%s_load_end = LOADADDR (.%s) + SIZEOF (.%s);\n", symbol, symbol, symbol;
        printf "   __overlay_load_offset += __%s_size;\n", symbol;
    }

    printf "\n";
    printf "   __overlay_load_size = __overlay_load_offset - __lma_end;\n";
}
