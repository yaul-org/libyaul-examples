#include "bg.h"

#include <string.h>

void
bg_calculate_params(bg_t *bg)
{
        bg->page_count = SCRN_CALCULATE_PAGE_COUNT(&bg->format);
        bg->page_width = SCRN_CALCULATE_PAGE_WIDTH(&bg->format);
        bg->page_height = SCRN_CALCULATE_PAGE_HEIGHT(&bg->format);
        bg->page_size = SCRN_CALCULATE_PAGE_SIZE(&bg->format);

        /* Maximum number of pages per planes is 4 */
        uint32_t page_idx = 0;
        for (page_idx = 0; page_idx < 4; page_idx++) {
                uint32_t offset = (page_idx & (bg->page_count - 1)) * (bg->page_size >> 1);

                /* XXX: Support case for 2x1 and 2x2 plane size (2 and 4 pages per plane, respectively) */
                bg->a_pages[page_idx] = (uint16_t *)(bg->format.scf_map.plane_a | offset);
                bg->b_pages[page_idx] = (uint16_t *)(bg->format.scf_map.plane_b | offset);
                bg->c_pages[page_idx] = (uint16_t *)(bg->format.scf_map.plane_c | offset);
                bg->d_pages[page_idx] = (uint16_t *)(bg->format.scf_map.plane_d | offset);
        }

        bg->cp_table = (uint16_t *)bg->format.scf_cp_table;
        bg->color_palette = (uint16_t *)bg->format.scf_color_palette;
}

void
bg_clear(const bg_t *bg)
{
        /* Number of planes is either 4 (NBGX) or 16 (RGBX) */
        uint32_t plane_idx;
        for (plane_idx = 0; plane_idx < 4; plane_idx++) {
                uint32_t page_idx = 0;
                for (page_idx = 0; page_idx < bg->page_count; page_idx++) {
                        memset(bg->a_pages[page_idx], 0x00, bg->page_size);
                        memset(bg->b_pages[page_idx], 0x00, bg->page_size);
                        memset(bg->c_pages[page_idx], 0x00, bg->page_size);
                        memset(bg->d_pages[page_idx], 0x00, bg->page_size);
                }
        }
}
