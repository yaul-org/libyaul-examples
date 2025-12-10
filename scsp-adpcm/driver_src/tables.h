#ifndef TABLES_H
#define TABLES_H

#define FILTER_TAB_COUNT 16

// 2048 = 1.0
// Must be quantities that can fit into a signed 13-bit integer
static const int filter_tab[FILTER_TAB_COUNT][3] =
{
 //
 // These first two filters must remain unaltered, due to hardcoded
 // assumptions in the code.
 //
 {  1920,     0,     0 },
 {     0,     0,     0 },
 //
 //
 {  3680, -1664,     0 },
 {  3136, -1760,     0 },
 {  3904, -1920,     0 },
 {  3136, -2368,  1248 },
 {  2976, -1472,   544 },
 {  3680, -2880,  1280 },
 {  3168, -2592,  1440 },
 {  1728,  -832,   896 },
 {  1856,   288,  -128 },
 {  2144,  -288,  -512 },
 {  2976,  -992,   256 },
 {  2144,  -480,   256 },
 {  4095, -2048,   -32 },
 {  3616, -3584,  1792 },
};

// Must be quantities that can fit into a signed 13-bit integer
static const int scale_tab[16] =
{
 1, 2, 3, 5, 9, 16, 28, 48, 84, 147, 256, 446, 776, 1351, 2352, 4095
};


//
// Change #if 0 to #if 1 if you want a low amplitude 22050Hz tone instead of a
// much higher amplitude, but constant, DC bias after 1-bit or 2-bit sample
// playback on a channel ends.
//
static const uint8 encoded_byte_xor_tab[3] =
{
#if 0
 0x00, 0xCC, 0xAA
#else
 0x00, 0x00, 0x00
#endif
};

#endif
