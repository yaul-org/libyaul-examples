/*
 * Copyright (c) 2012-2019 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <sys/cdefs.h>

#include <assert.h>
#include <math.h>

#include "scene.h"

typedef enum {
    SEEK_BEGIN,
    SEEK_CURRENT
} seek_origin_t;

struct frame_flags {
    unsigned int :5;
    unsigned int index_mode:1;
    unsigned int contains_palette_data:1;
    unsigned int clear_screen:1;
} __packed;

enum polygon_descriptor_flags {
    FRAME_END             = -1,
    FRAME_END_STREAM_SKIP = -2,
    STREAM_END            = -3
} __packed;

union polygon_descriptor {
    struct {
        unsigned int palette_index:4;
        unsigned int vertex_count:4;
    } __packed encoded;

    polygon_descriptor_flags flag;
} __attribute__ ((packed));

static_assert(sizeof(frame_flags) == 1);
static_assert(sizeof(polygon_descriptor) == 1);

static constexpr uint32_t _frame_count              = 1800;
static constexpr size_t _chunk_size                 = 64 * 1024; // 64 KiB chunk
static constexpr size_t _buffer_size                = _frame_count * _chunk_size;
static constexpr uint32_t _palette_count            = 16;
static constexpr size_t _vertex_buffer_size         = 256;
static constexpr size_t _polygon_vertex_buffer_size = 16;

static struct {
    const uint8_t* buffer;
    uint32_t offset;
    uint32_t chunk_index;
    uint32_t frame_index;
} __aligned(16) _state;

static uint8_vec2_t _vertex_buffer[8] __aligned(16);

static scene::start_handler _on_start;
static scene::end_handler _on_end;
static scene::update_palette_handler _on_update_palette;
static scene::clear_screen_handler _on_clear_screen;
static scene::draw_handler _on_draw;

static void _align(void);

static void _palette_update(void);
static uint32_t _palette_indices_get(int8_t(& palette_indices)[_palette_count]);

static void _vertex_buffer_indexed_get(const uint8_vec2_t* indexed_vertex_buffer,
                                       uint32_t vertex_count);

static inline void _buffer_seek(int32_t position, seek_origin_t whence) {
    switch (whence) {
    case SEEK_BEGIN:
        _state.offset = position;
        break;
    case SEEK_CURRENT:
        _state.offset += position;
        break;
    }
}

template<typename T>
static inline const T _buffer_read(void) {
    const auto buffer_ptr = &_state.buffer[_state.offset];
    const T* ptr_value = reinterpret_cast<const T*>(buffer_ptr);

    _buffer_seek(sizeof(T), SEEK_CURRENT);

    return *ptr_value;
}

template<typename T>
static inline const T* _buffer_tell(void) {
    const auto buffer_ptr = &_state.buffer[_state.offset];
    const T* ptr_value = reinterpret_cast<const T*>(buffer_ptr);

    return ptr_value;
}

void scene::init(const uint8_t*& buffer, const callbacks& callbacks) {
    _state.buffer = buffer;

    _on_start = ((callbacks.on_start != nullptr)
                 ? callbacks.on_start
                 : [] (uint32_t, bool) { });

    _on_end = ((callbacks.on_end != nullptr)
               ? callbacks.on_end
               : [] (uint32_t, bool) { });

    _on_update_palette = ((callbacks.on_update_palette != nullptr)
                          ? callbacks.on_update_palette
                          : [] (uint8_t, const rgb444) { });

    _on_clear_screen = ((callbacks.on_clear_screen != nullptr)
                        ? callbacks.on_clear_screen
                        : [] (bool) { });

    _on_draw = ((callbacks.on_draw != nullptr)
                ? callbacks.on_draw
                : [] (uint8_vec2_t const*, uint32_t, uint32_t) { });

    reset();
}

void scene::reset(void) {
    _buffer_seek(0, SEEK_BEGIN);

    _state.chunk_index = 0;
    _state.frame_index = 0;
}

void scene::process_frame(void) {
    _on_start(_state.frame_index, _state.frame_index == 0);

    auto frame_flags = _buffer_read<::frame_flags>();

    _on_clear_screen(frame_flags.clear_screen);

    if (frame_flags.contains_palette_data) {
        _palette_update();
    }

    const uint8_vec2_t* indexed_vertex_buffer = nullptr;

    if (frame_flags.index_mode) {
        uint8_t vertex_count = _buffer_read<uint8_t>();

        indexed_vertex_buffer = _buffer_tell<uint8_vec2_t>();

        _buffer_seek(vertex_count * sizeof(uint8_vec2_t), SEEK_CURRENT);
    }

    while (true) {
        auto polygon_descriptor = _buffer_read<::polygon_descriptor>();

        if (polygon_descriptor.flag == polygon_descriptor_flags::FRAME_END) {
            _on_end(_state.frame_index, false);

            break;
        }

        if (polygon_descriptor.flag == polygon_descriptor_flags::FRAME_END_STREAM_SKIP) {
            _on_end(_state.frame_index, false);
            _align();

            break;
        }

        if (polygon_descriptor.flag == polygon_descriptor_flags::STREAM_END) {
            _on_end(_state.frame_index, true);

            break;
        }

        const uint8_vec2_t* vertex_buffer;
        const uint8_t vertex_count = polygon_descriptor.encoded.vertex_count;

        if (frame_flags.index_mode) {
            _vertex_buffer_indexed_get(indexed_vertex_buffer, vertex_count);

            vertex_buffer = _vertex_buffer;
        } else {
            vertex_buffer = _buffer_tell<uint8_vec2_t>();

            _buffer_seek((uint32_t)vertex_count * sizeof(uint8_vec2_t), SEEK_CURRENT);
        }

        const uint8_t palette_index = polygon_descriptor.encoded.palette_index;

        _on_draw(vertex_buffer, vertex_count, palette_index);
    }

    _state.frame_index++;
}

static void _align(void) {
    _state.chunk_index++;

    _buffer_seek(_chunk_size * _state.chunk_index, SEEK_BEGIN);
}

static void _palette_update(void) {
    int8_t palette_indices[_palette_count];
    const uint32_t palette_index_count = _palette_indices_get(palette_indices);

    for (uint32_t offset = 0; offset < palette_index_count; offset++) {
        const uint8_t palette_index = palette_indices[offset];

        const uint8_t high_nybble = _buffer_read<uint8_t>();
        const uint8_t low_nybble = _buffer_read<uint8_t>();

        scene::rgb444 rgb;

        rgb.r = high_nybble & 0x0F;
        rgb.g = (low_nybble >> 4) & 0x0F;
        rgb.b = low_nybble & 0x0F;

        _on_update_palette(palette_index, rgb);
    }
}

static uint32_t _palette_indices_get(int8_t(& palette_indices)[_palette_count]) {
    const uint8_t high_nybble = _buffer_read<uint8_t>();
    const uint8_t low_nybble = _buffer_read<uint8_t>();
    const uint16_t mask = (high_nybble << 8) | low_nybble;

    uint32_t palette_index = 0;

    for (int32_t bit = 16 - 1; bit >= 0; bit--) {
        if (((mask >> bit) & 0x01) != 0x00) {
            uint8_t index = 15 - bit;

            palette_indices[palette_index] = index;

            palette_index++;
        }
    }

    return palette_index;
}

static void _vertex_buffer_indexed_get(const uint8_vec2_t* indexed_vertex_buffer,
                                       uint32_t vertex_count) {
    for (uint8_t i = 0; i < vertex_count; i++) {
        const uint8_t vertex_index = _buffer_read<uint8_t>();
        const uint8_vec2_t* const vertex = &indexed_vertex_buffer[vertex_index];

        _vertex_buffer[i].x = vertex->x;
        _vertex_buffer[i].y = vertex->y;
    }
}
