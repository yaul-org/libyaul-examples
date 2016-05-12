/*
 * Copyright (c) 2012-2014 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include "engine.h"

struct object_context;

TAILQ_HEAD(object_contexts, object_context);

struct object_context {
        /* Immediate parent */
        struct object *parent;
        struct object *object;
        /* Calculated absolute position of the object in world space */
        fix16_vector3_t position;
        struct object_contexts children;

        /* Tree management */
        TAILQ_ENTRY(object_context) tq_entries;
        /* For non-recursive traversal */
        SLIST_ENTRY(object_context) sl_entries;
} __aligned(32);

static bool _initialized = false;
/* Last tick when objects data was requested via objects_list() or
 * objects_sorted_list() */
static uint32_t _last_tick = 0;
static struct object_context *_root_ctx = NULL;

/* Caching */
/* Cached pointer to camera component */
static const struct component *_cached_camera = NULL;
/* Cached list of objects allocated */
static bool _cached_objects_dirty = true;
static struct objects _cached_objects;

static struct object_bucket_entry _obe_pool[OBJECTS_MAX];

MEMB(_object_context_pool, struct object_context, OBJECTS_MAX,
    sizeof(struct object_context));

static void add_objects_tree(struct object_context *, struct object *);
static void remove_objects_tree(struct object_context *);
static void clear_objects_tree(struct object_context *);
static void update_objects_tree(void);
static bool added_objects_tree(struct object_context *);

static void traverse_objects_tree(struct object_context *,
    void (*)(struct object_context *, void *), void *);

/* Visit functions */
static void visit_objects_tree_clear(struct object_context *, void *);
static void visit_objects_tree_update(struct object_context *, void *);

/*
 * Initialize objects system.
 */
void
objects_init(void)
{
        static struct transform transform = {
                .active = true,
                .id = COMPONENT_ID_TRANSFORM,
                .object = NULL,
                .position = FIX16_VECTOR3_INITIALIZER(0.0f, 0.0f, 0.0f)
        };

        /* "Null" object */
        static struct object root = {
                .active = true,
                .id = 0x4000,
                .component_list = {
                        OBJECT_COMPONENT_INITIALIZER(transform, &transform)
                },
                .component_count = 1
        };

        if (_initialized) {
                return;
        }

        memb_init(&_object_context_pool);

        /* Register root object */
        objects_object_register(&root);
        /* Get the root context */
        _root_ctx = (struct object_context *)root.context.instance;
        assert(_root_ctx != NULL);

        _initialized = true;
}

/*
 * Register object.
 *
 * Note, an object must be registered before it can be added to the
 * objects tree.
 */
void
objects_object_register(struct object *object)
{
        assert(object != NULL);
        assert(object->context.instance == NULL);

        /* Initialize context */
        struct object_context *object_ctx;
        object_ctx = (struct object_context *)memb_alloc(&_object_context_pool);
        assert(object_ctx != NULL);

        object_ctx->parent = NULL;
        object_ctx->object = object;

        fix16_vector3_zero(&object_ctx->position);

        TAILQ_INIT(&object_ctx->children);

        /* Connect context to object object */
        object->context.instance = object_ctx;
}

/*
 * Unregister object.
 */
void
objects_object_unregister(struct object *object)
{
        assert(object != NULL);
        assert(object->context.instance != NULL);

        struct object_context *object_ctx;
        object_ctx = (struct object_context *)object->context.instance;

        object_ctx->parent = NULL;
        object_ctx->object = NULL;

        /* Free context */
        int error __unused;
        error = memb_free(&_object_context_pool, object_ctx);
        assert(error == 0);

        /* Disconnect context from object */
        object->context.instance = NULL;
}

/*
 * Determine if the object is already present in the objects tree.
 */
bool
objects_object_added(const struct object *object)
{
        assert(object != NULL);
        /* Has the object been registered? */
        assert(object->context.instance != NULL);

        struct object_context *object_ctx;
        object_ctx = (struct object_context *)object->context.instance;

        return added_objects_tree(object_ctx);
}

/*
 * Add child object to root object.
 */
void
objects_object_add(struct object *object)
{
        assert(_initialized);
        assert(object != NULL);
        /* Has the object been registered? */
        assert(object->context.instance != NULL);

        add_objects_tree(_root_ctx, object);

        _cached_objects_dirty = true;
}

/*
 * Add child object to parent object.
 *
 * Note that there is no check that a back, forward, or cross edges (a
 * cycle) is introduced when adding a child object. When traversing the
 * tree, an infinite loop will occur should a cycle be present.
 */
void
objects_object_child_add(struct object *object, struct object *child)
{
        assert(_initialized);
        assert(object != NULL);
        assert(object->context.instance != NULL);
        assert(child != NULL);
        assert(child->context.instance != NULL);

        struct object_context *object_ctx;
        object_ctx = (struct object_context *)object->context.instance;

        add_objects_tree(object_ctx, child);

        _cached_objects_dirty = true;
}

/*
 * Remove object, but not its children.
 */
void
objects_object_remove(struct object *object)
{
        assert(_initialized);
        assert(object != NULL);
        assert(object->context.instance != NULL);

        struct object_context *object_ctx;
        object_ctx = (struct object_context *)object->context.instance;

        remove_objects_tree(object_ctx);

        _cached_objects_dirty = true;
}

/*
 * Remove object, but not its children.
 */
void
objects_object_clear(struct object *object)
{
        assert(_initialized);
        assert(object != NULL);
        assert(object->context.instance != NULL);

        struct object_context *object_ctx;
        object_ctx = (struct object_context *)object->context.instance;

        clear_objects_tree(object_ctx);

        _cached_objects_dirty = true;
}

/*
 *
 */
void
objects_clear(void)
{
        assert(_initialized);

        /* Clear cached */
        _cached_camera = NULL;

        /* Empty each bucket */
        uint32_t bucket_idx;
        for (bucket_idx = 0; bucket_idx < OBJECTS_Z_MAX_BUCKETS;
             bucket_idx++) {
                STAILQ_INIT(&_cached_objects.buckets[bucket_idx]);
        }
        /* Clear list */
        uint32_t object_idx;
        for (object_idx = 0; object_idx < OBJECTS_MAX; object_idx++) {
                _cached_objects.list[object_idx] = NULL;
        }

        _cached_objects_dirty = true;

        while (!(TAILQ_EMPTY(&_root_ctx->children))) {
                struct object_context *itr_ctx;
                itr_ctx = TAILQ_FIRST(&_root_ctx->children);

                struct object *itr_child;
                itr_child = itr_ctx->object;

                objects_object_clear(itr_child);
        }
}

/*
 * Return an array of objects in pre-order traversal from the objects
 * tree.
 */
const struct objects *
objects_fetch(void)
{
        assert(_initialized);

        bool new_frame;
        new_frame = (tick - _last_tick) != 0;

        if (!new_frame && !_cached_objects_dirty) {
                goto return_objects;
        }

        update_objects_tree();

        _cached_objects_dirty = false;

return_objects:
        _last_tick = tick;

        return (const struct objects *)&_cached_objects;
}

/*
 *
 */
const struct component *
objects_component_find(int32_t component_id)
{
        assert(component_id >= 1);

        /* As a sanity check, don't search for transform. Why would it
         * make any sense to search for a "random" transform
         * component? */
        assert(component_id != COMPONENT_ID_TRANSFORM);

        /* Caching */
        if ((component_id == COMPONENT_ID_CAMERA) && (_cached_camera != NULL)) {
                return _cached_camera;
        }

        const struct objects *objects;
        objects = objects_fetch();

        uint32_t object_idx;
        for (object_idx = 0; objects->list[object_idx] != NULL; object_idx++) {
                const struct object *object;
                object = objects->list[object_idx];

                const struct component *component;
                component = object_component_find(object, component_id);

                /* Caching */
                if (component_id == COMPONENT_ID_CAMERA) {
                        _cached_camera = component;
                }

                if (component != NULL) {
                        return component;
                }
        }

        return NULL;
}

/*
 *
 */
static void
add_objects_tree(struct object_context *parent_ctx, struct object *child)
{
        assert(child != NULL);
        assert(parent_ctx != NULL);
        assert(child->context.instance != NULL);

        struct object *parent;
        parent = parent_ctx->object;
        assert(parent != NULL);

        struct object_context *child_ctx;
        child_ctx = (struct object_context *)child->context.instance;
        assert(child_ctx != NULL);

        child_ctx->parent = parent;
        child_ctx->object = child;
        fix16_vector3_zero(&child_ctx->position);

        TAILQ_INSERT_TAIL(&parent_ctx->children, child_ctx, tq_entries);
}

/*
 *
 */
static bool
added_objects_tree(struct object_context *object_ctx)
{
        assert(object_ctx != NULL);
        assert(object_ctx->object != NULL);
        assert(object_ctx->object->context.instance == object_ctx);

        struct object_context *top_object_ctx;
        top_object_ctx = object_ctx;

        while (top_object_ctx != _root_ctx) {
                const struct object *parent;
                parent = top_object_ctx->parent;
                /* We've reached the root of the objects tree
                 *
                 * Or this entire traversal was performed on a dangling
                 * objects sub-tree */
                if (parent == NULL) {
                        return false;
                }

                struct object_context *parent_ctx;
                parent_ctx = (struct object_context *)parent->context.instance;

                /* Traverse up the objects tree */
                top_object_ctx = parent_ctx;
        }

        return true;
}

/*
 * Remove object, but keep its children attached.
 */
static void
remove_objects_tree(struct object_context *remove_ctx)
{
        assert(remove_ctx != NULL);
        assert(remove_ctx->object != NULL);
        assert(remove_ctx->object->context.instance == remove_ctx);
        /* The root node is not to be removed */
        assert(remove_ctx != _root_ctx);

        const struct object *parent;
        parent = remove_ctx->parent;
        assert(parent != NULL);

        struct object_context *parent_ctx;
        parent_ctx = (struct object_context *)parent->context.instance;
        assert(parent_ctx != NULL);

        /* Remove from objects tree */
        TAILQ_REMOVE(&parent_ctx->children, remove_ctx, tq_entries);
}

/*
 * Remove object and its children.
 */
static void
clear_objects_tree(struct object_context *object_ctx)
{
        traverse_objects_tree(object_ctx, visit_objects_tree_clear, NULL);
}

/*
 * Traverse objects tree to update each object's absolute position and
 * populate the Z buckets.
 */
static void
update_objects_tree(void)
{
        /* Empty each bucket */
        uint32_t bucket_idx;
        for (bucket_idx = 0; bucket_idx < OBJECTS_Z_MAX_BUCKETS; bucket_idx++) {
                STAILQ_INIT(&_cached_objects.buckets[bucket_idx]);
        }
        /* Clear list */
        uint32_t object_idx;
        for (object_idx = 0; object_idx < OBJECTS_MAX; object_idx++) {
                _cached_objects.list[object_idx] = NULL;
        }

        /* Buffer for state */
        uint32_t args_buffer[2] = {
                0,
                0
        };

        traverse_objects_tree(_root_ctx, visit_objects_tree_update,
            &args_buffer);
}

static void
visit_objects_tree_clear(struct object_context *object_ctx,
    void *args __unused)
{
        remove_objects_tree(object_ctx);
}

static void
visit_objects_tree_update(struct object_context *object_ctx, void *args)
{
        struct {
                uint32_t obe_idx;
                uint32_t object_idx;
        } __packed *state;

        state = args;

        const struct object *parent;
        parent = object_ctx->parent;
        if (parent == NULL) {
                return;
        }

        struct object *object;
        object = object_ctx->object;
        /* Ensure that the context is connected to the right object */
        assert(object_ctx == object->context.instance);

        struct transform *transform;
        transform = (struct transform *)object_component_find(object,
            COMPONENT_ID_TRANSFORM);
        assert(transform != NULL);

        const struct object_context *parent_ctx;
        parent_ctx = parent->context.instance;
        assert(parent_ctx != NULL);

        /* Calculate the absolute position */
        fix16_vector3_add(&COMPONENT(transform, position),
            &parent_ctx->position,
            &object_ctx->position);

        /* Insert object into its corresponding Z "bucket" */
        int32_t z_position;
        z_position = (int32_t)fix16_to_int(object_ctx->position.z);

        /* Make sure the Z position falls within the buckets */
        assert((z_position >= OBJECTS_Z_MIN) && (z_position <= OBJECTS_Z_MAX));

        /* Allocate object bucket entry */
        struct object_bucket_entry *object_bucket_entry;
        object_bucket_entry = &_obe_pool[state->obe_idx];
        state->obe_idx++;

        /* Populate Z bucket */
        object_bucket_entry->object = object;
        object_bucket_entry->position = &object_ctx->position;
        STAILQ_INSERT_TAIL(&_cached_objects.buckets[z_position],
            object_bucket_entry, entries);

        /* Populate list */
        _cached_objects.list[state->object_idx] = object;
        state->object_idx++;
}

/*
 *
 */
static void
traverse_objects_tree(struct object_context *object_ctx,
    void (*visit)(struct object_context *, void *), void *args)
{
        assert(object_ctx != NULL);
        assert(object_ctx->object->context.instance == object_ctx);

        SLIST_HEAD(stack, object_context) stack = SLIST_HEAD_INITIALIZER(stack);
        SLIST_INIT(&stack);

        SLIST_INSERT_HEAD(&stack, object_ctx, sl_entries);
        while (!(SLIST_EMPTY(&stack))) {
                struct object_context *top_object_ctx;
                top_object_ctx = SLIST_FIRST(&stack);

                SLIST_REMOVE_HEAD(&stack, sl_entries);

                assert(top_object_ctx != NULL);
                assert(top_object_ctx->object->context.instance == top_object_ctx);

                visit(top_object_ctx, args);

                struct object_contexts *children;
                children = &top_object_ctx->children;

                struct object_context *itr_child_ctx;
                TAILQ_FOREACH_REVERSE (itr_child_ctx, children, object_contexts,
                    tq_entries) {
                        SLIST_INSERT_HEAD(&stack, itr_child_ctx, sl_entries);
                }
        }
}
