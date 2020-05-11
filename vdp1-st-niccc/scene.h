#ifndef SCENE_H_
#define SCENE_H_

#include <stdint.h>

#include <fix16.h>

namespace scene {
    union rgb444 {
        struct {
            unsigned int  :4;
            unsigned int r:4;
            unsigned int g:4;
            unsigned int b:4;
        } __attribute__ ((packed));

        uint16_t value;
    };

    static_assert(sizeof(rgb444) == 2);

    typedef void (*start_handler)(uint32_t frame_index, bool first_frame);
    typedef void (*end_handler)(uint32_t frame_index, bool last_frame);
    typedef void (*update_palette_handler)(uint8_t palette_index,
                                           const rgb444 color);
    typedef void (*clear_screen_handler)(bool clear_screen);
    typedef void (*draw_handler)(int16_vec2_t const* vertex_buffer,
                                 const size_t count,
                                 const uint8_t palette_index);

    struct callbacks {
        start_handler on_start;
        end_handler on_end;
        update_palette_handler on_update_palette;
        clear_screen_handler on_clear_screen;
        draw_handler on_draw;
    };

    void init(const uint8_t*& buffer, const callbacks& callbacks);
    void reset(void);

    void process_frame(void);
};

#endif // SCENE_H_
