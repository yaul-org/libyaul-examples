/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include "../engine.h"

#define STATES_MAX 16

struct scene;

TAILQ_HEAD(scenes, scene);

struct scene {
        struct scene_ctx scene_ctx;

        const char *scene_name;

        uint32_t scene_id;

        void (*scene_init)(struct scene_ctx *);
        void (*scene_update)(struct scene_ctx *);
        void (*scene_draw)(struct scene_ctx *);
        void (*scene_exit)(struct scene_ctx *);

        TAILQ_ENTRY(scene) entries;
};

static bool _initialized = false;
static struct scenes _scenes;
static struct scene *_prev_scene = NULL;
static struct scene *_cur_scene = NULL;

void
scene_init(void)
{
        if (_initialized) {
                return;
        }

        TAILQ_INIT(&_scenes);

        _prev_scene = NULL;
        _cur_scene = NULL;

        _initialized = true;
}

void
scene_add(const char *scene_name, uint32_t scene_id,
    void (*scene_init)(struct scene_ctx *),
    void (*scene_update)(struct scene_ctx *),
    void (*scene_draw)(struct scene_ctx *),
    void (*scene_exit)(struct scene_ctx *),
    void *data)
{
        assert(_initialized);

        struct scene *scene;
        scene = (struct scene *)malloc(sizeof(struct scene));
        assert(scene != NULL);

        scene->scene_name = scene_name;
        scene->scene_id = scene_id;
        scene->scene_init = scene_init;
        scene->scene_update = scene_update;
        scene->scene_draw = scene_draw;
        scene->scene_exit = scene_exit;

        /* Initialize scene context */
        scene->scene_ctx.sc_frames = 0;
        scene->scene_ctx.sc_data = data;

        TAILQ_INSERT_TAIL(&_scenes, scene, entries);
}

void
scene_transition(uint32_t scene_id)
{
        assert(_initialized);

        _prev_scene = _cur_scene;

        struct scene *prev_scene;
        prev_scene = _prev_scene;
        if (prev_scene != NULL) {
                if (prev_scene->scene_exit != NULL) {
                        prev_scene->scene_exit(&prev_scene->scene_ctx);
                }

                prev_scene->scene_ctx.sc_frames = 0;
        }

        /* Check if transitioning scene is valid */
        struct scene *scene;

        bool scene_found __unused;
        scene_found = false;

        TAILQ_FOREACH (scene, &_scenes, entries) {
                if (scene->scene_id == scene_id) {
                        scene_found = true;
                        break;
                }
        }
        assert(scene_found);
        _cur_scene = scene;

        struct scene *cur_scene;
        cur_scene = _cur_scene;

        if (cur_scene->scene_init != NULL) {
                cur_scene->scene_init(&cur_scene->scene_ctx);
        }
}

uint32_t
scene_current(void)
{
        assert(_initialized);

        struct scene *scene;
        scene = _cur_scene;

        return scene->scene_id;
}

void
scene_handler_update(void)
{
        assert(_initialized);

        struct scene *scene;
        scene = _cur_scene;

        scene->scene_ctx.sc_frames++;

        if (scene->scene_update != NULL) {
                scene->scene_update(&scene->scene_ctx);
        }
}

void
scene_handler_draw(void)
{
        assert(_initialized);

        struct scene *scene;
        scene = _cur_scene;

        if (scene->scene_ctx.sc_frames == 0) {
                return;
        }

        if (scene->scene_draw != NULL) {
                scene->scene_draw(&scene->scene_ctx);
        }
}
