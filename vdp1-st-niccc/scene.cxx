/*
 * Copyright (c) 2012-2019 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <assert.h>
#include <math.h>

#include "scene.h"

struct frame_flags {
    unsigned int :5;
    unsigned int index_mode:1;
    unsigned int contains_palette_data:1;
    unsigned int clear_screen:1;
} __attribute__ ((packed));

struct uint8_vector {
    uint8_t x;
    uint8_t y;
} __attribute__ ((packed));

enum polygon_descriptor_flags {
    FRAME_END = -1,
    FRAME_END_STREAM_SKIP = -2,
    STREAM_END = -3
} __attribute__ ((packed));

union polygon_descriptor {
    struct {
        unsigned int palette_index:4;
        unsigned int vertex_count:4;
    } __attribute__ ((packed)) encoded;

    polygon_descriptor_flags flag;
} __attribute__ ((packed));

static_assert(sizeof(frame_flags) == 1);
static_assert(sizeof(polygon_descriptor) == 1);

static_assert(sizeof(uint8_vector) == 2);

static constexpr uint32_t _frame_count = 1800;
static constexpr size_t _chunk_size = 64 * 1024; // 64 KiB chunk
static constexpr size_t _buffer_size = _frame_count * _chunk_size;
static constexpr uint32_t _palette_count = 16;
static constexpr size_t _vertex_buffer_size = 256;
static constexpr size_t _polygon_vertex_buffer_size = 16;

static const uint8_t* _buffer = nullptr;
static uint32_t _offset = 0;
static uint32_t _chunk_index = 0;
static uint32_t _frame_index = 0;
static uint8_vector* _indexed_vertex_buffer;
static int16_vector2_t* _vertex_buffer;

static scene::start_handler _on_start;
static scene::end_handler _on_end;
static scene::update_palette_handler _on_update_palette;
static scene::clear_screen_handler _on_clear_screen;
static scene::draw_handler _on_draw;

static void _align(void);

static void _palette_init(void);
static void _palette_update(void);
static uint32_t _palette_indices_get(int8_t(& palette_indices)[_palette_count]);

static void _vertex_buffer_init(void);
static void _indexed_vertex_buffer_get(void);
static uint8_t _vertex_buffer_indexed_get(const polygon_descriptor polygon_descriptor);
static uint8_t _vertex_buffer_get(const polygon_descriptor polygon_descriptor);

template<typename T>
static inline const T _read(void) {
    auto buffer_ptr = &_buffer[_offset];
    const T* ptr_value = reinterpret_cast<const T*>(buffer_ptr);

    _offset++;

    return *ptr_value;
}

void scene::init(const uint8_t*& buffer, const callbacks& callbacks) {
    _buffer = buffer;

    _on_start = ((callbacks.on_start != nullptr)
                 ? callbacks.on_start
                 : [] (uint32_t, bool) { });

    _on_end = ((callbacks.on_end != nullptr)
               ? callbacks.on_end
               : [] (uint32_t, bool) { });

    _on_update_palette = ((callbacks.on_update_palette != nullptr)
                          ? callbacks.on_update_palette
                          : [](uint8_t, const rgb444) { });

    _on_clear_screen = ((callbacks.on_clear_screen != nullptr)
                        ? callbacks.on_clear_screen
                        : [] (bool) { });

    _on_draw = ((callbacks.on_draw != nullptr)
                ? callbacks.on_draw
                : [](int16_vector2_t const*, size_t, uint8_t) { });

    _palette_init();
    _vertex_buffer_init();

    reset();
}

void scene::reset(void) {
    _offset = 0;
    _chunk_index = 0;
    _frame_index = 0;
}

void scene::process_frame(void) {
    _on_start(_frame_index, _offset == 0);

    auto frame_flags = _read<::frame_flags>();

    _on_clear_screen(frame_flags.clear_screen);

    if (frame_flags.contains_palette_data) {
        _palette_update();
    }

    if (frame_flags.index_mode) {
        _indexed_vertex_buffer_get();
    }

    while (true) {
        auto polygon_descriptor = _read<::polygon_descriptor>();

        if (polygon_descriptor.flag == polygon_descriptor_flags::FRAME_END) {
            _on_end(_frame_index, false);

            break;
        } else if (polygon_descriptor.flag == polygon_descriptor_flags::FRAME_END_STREAM_SKIP) {
            _on_end(_frame_index, false);
            _align();

            break;
        } else if (polygon_descriptor.flag == polygon_descriptor_flags::STREAM_END) {
            _on_end(_frame_index, true);

            break;
        } else {
            uint8_t vertex_count;

            if (frame_flags.index_mode) {
                vertex_count = _vertex_buffer_indexed_get(polygon_descriptor);
            } else {
                vertex_count = _vertex_buffer_get(polygon_descriptor);
            }

            uint8_t palette_index = polygon_descriptor.encoded.palette_index;

            _on_draw(_vertex_buffer, vertex_count, palette_index);
        }
    }

    _frame_index++;
}

static void _align(void) {
    _chunk_index++;
    _offset = _chunk_size * _chunk_index;
}

static void _palette_init(void) {
}

static void _palette_update(void) {
    int8_t palette_indices[_palette_count];
    const uint32_t palette_index_count = _palette_indices_get(palette_indices);

    for (uint32_t offset = 0; offset < palette_index_count; offset++) {
        const uint8_t palette_index = palette_indices[offset];

        const uint8_t high_nybble = _read<uint8_t>();
        const uint8_t low_nybble = _read<uint8_t>();

        scene::rgb444 rgb;

        rgb.r = high_nybble & 0x0F;
        rgb.g = (low_nybble >> 4) & 0x0F;
        rgb.b = low_nybble & 0x0F;

        _on_update_palette(palette_index, rgb);
    }
}

static uint32_t _palette_indices_get(int8_t(& palette_indices)[_palette_count]) {
    const uint8_t high_nybble = _read<uint8_t>();
    const uint8_t low_nybble = _read<uint8_t>();
    const uint16_t mask = (high_nybble << 8) | low_nybble;

    for (uint32_t index = 0; index < _palette_count; index++) {
        palette_indices[index] = -1;
    }

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

static void _vertex_buffer_init(void) {
    _indexed_vertex_buffer =  new uint8_vector[_vertex_buffer_size];
    assert(_indexed_vertex_buffer != nullptr);

    _vertex_buffer = new int16_vector2_t[_polygon_vertex_buffer_size];
    assert(_vertex_buffer != nullptr);
}

static void _indexed_vertex_buffer_get(void) {
    uint8_t vertex_count = _read<uint8_t>();

    for (uint8_t i = 0; i < vertex_count; i++) {
        const uint8_t x = _read<uint8_t>();
        const uint8_t y = _read<uint8_t>();

        _indexed_vertex_buffer[i].x = x;
        _indexed_vertex_buffer[i].y = y;
    }
}

static uint8_t _vertex_buffer_indexed_get(const polygon_descriptor polygon_descriptor) {
    const uint8_t vertex_count = polygon_descriptor.encoded.vertex_count;

    for (uint8_t i = 0; i < vertex_count; i++) {
        const uint8_t vertex_index = _read<uint8_t>();
        const uint8_vector& vertex = _indexed_vertex_buffer[vertex_index];

        _vertex_buffer[i].x = vertex.x;
        _vertex_buffer[i].y = vertex.y;
    }

    return vertex_count;
}

static uint8_t _vertex_buffer_get(const polygon_descriptor polygon_descriptor) {
    const uint8_t vertex_count = polygon_descriptor.encoded.vertex_count;

    for (uint8_t i = 0; i < vertex_count; i++) {
        const uint8_t x = _read<uint8_t>();
        const uint8_t y = _read<uint8_t>();

        _vertex_buffer[i].x = x;
        _vertex_buffer[i].y = y;
    }

    return vertex_count;
}
