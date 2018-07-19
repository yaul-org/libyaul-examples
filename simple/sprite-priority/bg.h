#ifndef BG_H
#define BG_H

#include <yaul.h>

typedef struct {
        struct scrn_cell_format format;

        uint16_t *cp_table;
        uint16_t *color_palette;

        uint16_t page_count;
        uint32_t page_width;
        uint32_t page_height;
        uint32_t page_size;

        uint16_t *a_pages[4];
        uint16_t *b_pages[4];
        uint16_t *c_pages[4];
        uint16_t *d_pages[4];
} bg_t;

void bg_calculate_params(bg_t *);
void bg_clear(const bg_t *);

#endif /* !BG_H */
