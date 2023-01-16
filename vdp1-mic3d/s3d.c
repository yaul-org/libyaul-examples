#include <assert.h>
#include <stdlib.h>

#include <mic3d.h>

#define PATCH_ADDRESS(s3d, x) ((void *)((uintptr_t)(s3d) + (uintptr_t)(x)))

#define ATTRIBUTE(f, s, t, c, g, a, d, o) {                                    \
        f, /* flag */                                                          \
        (s) | (((d) >> 16) & 0x1C) | (o), /* sort */                           \
        t, /* texno */                                                         \
        (a) | (((d) >> 24) & 0xC0), /* atrb */                                 \
        c, /* colno */                                                         \
        g, /* gstb */                                                          \
        (d) & 0x3F /* dir */                                                   \
}

typedef struct {
        char sig[4];
        uint32_t version;
        uint32_t flags;
        uint32_t object_count;

        /* TEXTURE *textures; */
        /* uint32_t textures_count; */
        unsigned int :32;
        unsigned int :32;

        /* PALETTE *palettes; */
        /* uint32_t palettes_count; */
        unsigned int :32;
        unsigned int :32;

        /* g3d_s3d_texture_t *texture_datas; */
        /* g3d_s3d_palette_t *palette_datas; */
        unsigned int :32;
        unsigned int :32;

        void *eof;
} __packed s3d_t;

typedef fix16_t POINT[3];
typedef fix16_t VECTOR[3];

typedef struct {
        VECTOR norm;
        uint16_t Vertices[4];
} POLYGON;

typedef struct {
        uint8_t flag;
        uint8_t sort;
        uint16_t texno;
        uint16_t atrb;
        uint16_t colno;
        uint16_t gstb;
        uint16_t dir;
} __packed ATTR;

typedef struct {
        POINT *pntbl;
        uint32_t nbPoint;
        POLYGON *pltbl;
        uint32_t nbPolygon;
        /* unsigned int :32; */
        ATTR *attbl;
        VECTOR *vntbl;
} __packed XPDATA;

typedef struct {
        XPDATA xpdata;

        /* PICTURE *pictures; */
        /* uint32_t picture_count; */
        unsigned int :32;
        unsigned int :32;

        unsigned int :32;
        unsigned int :32;
        /* vdp1_gouraud_table_t *gouraud_tables; */
        /* uint32_t gouraud_table_count; */
} __packed s3d_object_t;

static s3d_object_t *
_s3d_objects_get(const s3d_t *s3d)
{
        return (void *)((uintptr_t)s3d + sizeof(s3d_t));
}

static void
_object_patch(s3d_t *s3d, s3d_object_t *object)
{
        object->xpdata.pntbl = PATCH_ADDRESS(s3d, object->xpdata.pntbl);
        object->xpdata.pltbl = PATCH_ADDRESS(s3d, object->xpdata.pltbl);
        object->xpdata.attbl = PATCH_ADDRESS(s3d, object->xpdata.attbl);
        object->xpdata.vntbl = PATCH_ADDRESS(s3d, object->xpdata.vntbl);
        /* object->gouraud_tables = PATCH_ADDRESS(s3d, object->gouraud_tables); */
        /* object->pictures = PATCH_ADDRESS(s3d, object->pictures); */
}

void
s3d_read(void *ptr, mesh_t *mesh)
{
        s3d_t * const s3d = ptr;

        s3d_object_t * const s3d_objects = _s3d_objects_get(s3d);

        s3d->eof = PATCH_ADDRESS(s3d, s3d->eof);

        s3d_object_t * const object = &s3d_objects[0];

        _object_patch(s3d, object);

        mesh->points = (fix16_vec3_t *)object->xpdata.pntbl;
        mesh->points_count = object->xpdata.nbPoint;
        attribute_t * const attributes =
            malloc(sizeof(attribute_t) * object->xpdata.nbPolygon);
        assert(attributes != NULL);

        mesh->attributes = attributes;

        for (uint32_t i = 0; i < object->xpdata.nbPolygon; i++) {
                attribute_t * const attribute = &attributes[i];
                const ATTR * const sgl_attr = &object->xpdata.attbl[i];

                attribute->draw_mode.raw = sgl_attr->atrb;
                attribute->control.sort_type = sgl_attr->sort & 3;
                attribute->control.read_dir = (sgl_attr->dir >> 4) & 3;
                attribute->control.use_texture = (sgl_attr->sort >> 2) & 1;
                attribute->control.use_lighting = (sgl_attr->sort >> 3) & 1;
                attribute->control.command = sgl_attr->dir & 0xF;

                attribute->base_color.raw = sgl_attr->colno;
                attribute->texture_slot = sgl_attr->texno;
                attribute->shading_slot = sgl_attr->gstb;
        }

        mesh->polygons = (polygon_t *)object->xpdata.pltbl;
        mesh->polygons_count = object->xpdata.nbPolygon;
}
