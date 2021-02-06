# 0000  4B S3D '\0'
# 0004  2B Version number
# 0006  4B Flags
# 0010  2B XPDATA count                                     XPDATA will always be at offset 64
# 0012  2B PICTURE count
# .
# .
# .
# 0060  4B User offset to data
# 0064     XPDATA 0                                         Each has offsets to their data
# ....     XPDATA 1
# ....     XPDATA 2
# ....
# ....     AUX 0
# ....     AUX 1
# ....     AUX 2
# ....
# ....     PICTURE 0
# ....     PICTURE 1
# ....     PICTURE 2
# ....
# ....     TEXTURE 0
# ....     TEXTURE 1
# ....     TEXTURE 2
################################################################################
# 1. Bounding boxes are missing
#    Allow flexibility for per object BB or entire BB
# 2. ATTRIBUTE panel
# 3. Refactor into multiple PY files
################################################################################
import ctypes
import math
import os
import struct
import sys

from enum import Enum, IntFlag

import bpy
import bpy_extras
import bmesh
import mathutils
import pathlib
import gpu

from bpy.app.handlers import persistent

# fmt: off
bl_info = {
    "name": "Export Sega Saturn Model (.ssm)",
    "category": "Import-Export"
}
# fmt: on

# SGL


class SortType(Enum):
    SORT_BFR = 0
    SORT_MIN = 1
    SORT_MAX = 2
    SORT_CEN = 3


class ColorType(Enum):
    RGB = 0
    CLUT_INDEX = 1
    COLOR_BANK = 2


class PlaneType(Enum):
    SINGLE_PLANE = 0
    DUAL_PLANE = 1


class AttributeFlags(IntFlag):
    MSB_ON = 1 << 15
    HSS_ON = 1 << 12
    HSS_OFF = 0 << 12
    NO_WINDOW = 0 << 9
    WINDOW_IN = 2 << 9
    WINDOW_OUT = 3 << 9
    MESH_OFF = 0 << 8
    MESH_ON = 1 << 8
    EC_DISABLE = 1 << 7
    EC_ENABLE = 0 << 7
    SP_DISABLE = 1 << 6
    SP_ENABLE = 0 << 6
    CL_16_BANK = 0 << 3
    CL_16_LOOK = 1 << 3
    CL_64_BANK = 2 << 3
    CL_128_BANK = 3 << 3
    CL_256_BANK = 4 << 3
    CL_32K_RGB = 5 << 3
    CL_REPLACE = 0
    CL_SHADOW = 1
    CL_HALF = 2
    CL_TRANS = 3
    CL_GOURAUD = 4


class PrimitiveType(Enum):
    NORMAL_SPRITE = 0
    SCALED_SPRITE = 1
    DISTORTED_SPRITE = 2
    POLYGON = 4
    POLYLINE = 5
    LINE = 6
    SYSTEM_CLIP = 9
    USER_CLIP = 8
    BASE_POSITION = 10
    END = -1


class FlipDir(IntFlag):
    NONE = 0
    H = 1 << 4
    V = 1 << 5


class FaceData(object):
    def __init__(self):
        self.indices = []
        self.normal = mathutils.Vector((0.0, 0.0, 1.0))
        self.colors = []
        self.attribute = Attribute(self)


class VertexData(object):
    def __init__(self):
        self.point = mathutils.Vector((0.0, 0.0, 0.0))


class ColorData(object):
    def __init__(self):
        self.color_type = ColorType.RGB
        self.data = mathutils.Color((0.5, 0.5, 0.5))


class Attribute(object):
    BYTE_SIZE = 1 + 1 + (2 * 5)

    def __init__(self, face_data):
        self.face_data = face_data
        self.plane_type = PlaneType.SINGLE_PLANE
        self.sort_type = SortType.SORT_CEN
        self.texture_num = 0
        self.color_data = ColorData()
        self.gouraud_table_num = 0
        self.attribute_flags = AttributeFlags.MESH_OFF
        self.primitive_type = PrimitiveType.POLYGON
        self.flip_dir = FlipDir.NONE


class XPDATA(object):
    # XPDATA -> 24B
    #   POINT *   pntbl     4B offset
    #   Uint32    nbPoint   4B
    #   POLYGON * pltbl     4B offset
    #   Uint32    nbPolygon 4B
    #   ATTR *    attbl     4B offset
    #   VECTOR *  vntbl     4B offset
    BYTE_SIZE = 6 * 4

    def __init__(self):
        self.vertex_datas = []
        self.face_datas = []


class AUX(object):
    # AUX -> 16B
    #   void *   Gouraud table       4B offset
    #   Uint16   Gouraud table count 4B
    #   void *   CG                  4B offset
    #   Uint32   CG size             4B
    #   Uint16   ------------------- 2B padding
    BYTE_SIZE = 4 + 2 + 4 + 4 + 2


class GouraudTable(object):
    def __init__(self):
        self._index = -1

    @property
    def index(self):
        return self._index

    def allocate_index(self):
        self._index += 1


################################################################################
# Packing

VECTOR_BYTE_SIZE = 4 * 3
POLYGON_BYTE_SIZE = VECTOR_BYTE_SIZE + (4 * 2)


class S3DFlags(IntFlag):
    NONE = 0


class PackedXPDATA(object):
    def __init__(self):
        self.xpdata = XPDATA()
        self.packed_points = b""
        self.packed_polygons = b""
        self.packed_attributes = b""
        self.packed_normals = b""
        self.packed_gouraud_tables = b""


class S3D(object):
    BYTE_SIZE = 64

    VERSION = 1

    def __init__(self):
        self.packed_xpdatas = []
        self.packed_pictures = []
        self.packed_textures = []
        self.flags = S3DFlags.NONE
        self.gouraud_table_num = 0

    @property
    def pools_offset(self):
        return (
            S3D.BYTE_SIZE
            + s3d.total_xpdatas_size
            + s3d.total_auxs_size
            + s3d.total_pictures_size
            + s3d.total_textures_size
        )

    @property
    def total_xpdatas_size(self):
        return len(s3d.packed_xpdatas) * XPDATA.BYTE_SIZE

    @property
    def total_auxs_size(self):
        return len(s3d.packed_xpdatas) * AUX.BYTE_SIZE

    @property
    def total_pictures_size(self):
        return 0

    @property
    def total_textures_size(self):
        return 0

    @property
    def total_points_size(self):
        return sum([len(p.packed_points) for p in self.packed_xpdatas])

    @property
    def total_polygons_size(self):
        return sum([len(p.packed_polygons) for p in self.packed_xpdatas])

    @property
    def total_attributes_size(self):
        return sum([len(p.packed_attributes) for p in self.packed_xpdatas])

    @property
    def total_normals_size(self):
        return sum([len(p.packed_normals) for p in self.packed_xpdatas])


################################################################################
# Utilities


def to_fix16(value):
    return int((value * 65536.0) + (0.5 if (value >= 0) else -0.5))


def to_fix16_vector(vector):
    if not isinstance(vector, mathutils.Vector):
        raise TypeError("Expected mathutils.Vector, got %s", (type(vector)))
    return tuple([to_fix16(component) for component in vector])


def to_rgb1555(color):
    def clamp_color_component(value):
        return min(value, 1.0)

    r = int(clamp_color_component(color[0]) * 255)
    g = int(clamp_color_component(color[1]) * 255)
    b = int(clamp_color_component(color[2]) * 255)

    byte_r = (r >> 3) & 0x1F
    byte_g = (g >> 3) & 0x1F
    byte_b = (b >> 3) & 0x1F

    return 0x8000 | (byte_b << 10) | (byte_g << 5) | (byte_r)


def flatten(lsts):
    return [item for sublist in lsts for item in sublist]


def align_offset(offset, boundary_bit=2):
    boundary_align = 1 << boundary_bit
    return int(math.ceil(offset / float(boundary_align)) * boundary_align)


################################################################################
# Blender utilities


def get_selected_faces(mesh):
    if not isinstance(mesh, bmesh.types.BMesh):
        raise TypeError("Expected bmesh.types.BMesh, got %s" % type(mesh))

    mesh.faces.ensure_lookup_table()

    return [face for face in mesh.faces if face.select]


################################################################################
# Mesh operations


def extract_faces(gouraud_table, obj, mesh):
    if not isinstance(gouraud_table, GouraudTable):
        raise TypeError("Expected GouraudTable, got %s" % (type(gouraud_table)))
    if not isinstance(obj, bpy.types.Object):
        raise TypeError("Expected bpy.types.Object, got %s" % type(obj))
    if not isinstance(mesh, bmesh.types.BMesh):
        raise TypeError("Expected bmesh.types.BMesh, got %s" % type(mesh))

    mesh.faces.ensure_lookup_table()
    mesh.verts.ensure_lookup_table()

    vertex_colors = []
    if len(obj.data.vertex_colors) != 0:
        print("len: %i" % (len(obj.data.vertex_colors)))
        color_layer = obj.data.vertex_colors[0]
        vertex_colors = [value.color for value in color_layer.data]

    face_datas = []

    for face in mesh.faces:
        indices = []
        if len(face.loops) == 3:
            indices = [loop.index for loop in face.loops[0:3]] + [face.loops[2].index]
        elif len(face.loops) == 4:
            indices = [loop.index for loop in face.loops[0:4]]
        else:
            assert False, "Invalid face vertex count: %i" % (len(face.loops))

        face_data = FaceData()
        face_data.indices = [obj.data.loops[index].vertex_index for index in indices]
        face_data.normal = face.normal.copy()

        if vertex_colors:
            gouraud_table.allocate_index()

            face_data.colors = [vertex_colors[index] for index in indices]

            # Update attribute
            face_data.attribute.attribute_flags |= AttributeFlags.CL_GOURAUD
            face_data.attribute.gouraud_table_num = gouraud_table.index

        # Perform a test to see if the winding is correct
        vertices = [mesh.verts[index].co for index in face_data.indices]

        u1 = (vertices[3] - vertices[0]).normalized()
        v1 = (vertices[1] - vertices[0]).normalized()
        n1 = u1.cross(v1)
        dir1 = face_data.normal.dot(n1)

        u2 = (vertices[3] - vertices[2]).normalized()
        v2 = (vertices[1] - vertices[2]).normalized()
        n2 = u2.cross(v2)
        dir2 = face_data.normal.dot(n2)

        if (not math.isclose(dir1, 1.0, abs_tol=0.001)) and (
            not math.isclose(dir2, 1.0, abs_tol=0.001)
        ):
            print("%s -> %s/%s" % (face_data.normal, n1, n2))

        face_datas.append(face_data)

    return face_datas


def extract_vertices(mesh):
    if not isinstance(mesh, bmesh.types.BMesh):
        raise TypeError("Expected bmesh.types.BMesh, got %s" % (type(mesh)))

    mesh.verts.ensure_lookup_table()

    vertex_datas = []

    for vert in mesh.verts:
        vertex_data = VertexData()
        vertex_data.point = vert.co.copy()

        vertex_datas.append(vertex_data)

    return vertex_datas


################################################################################
# Packing


def pack_points(vertex_datas):
    points_bytes = []

    for vertex_data in vertex_datas:
        fix16_vector = to_fix16_vector(vertex_data.point)
        points_bytes.append(fix16_vector[0])
        points_bytes.append(fix16_vector[1])
        points_bytes.append(fix16_vector[2])

    struct_format = ">%il" % (len(points_bytes))

    return struct.pack(struct_format, *points_bytes)


def pack_polygons(face_datas):
    struct_format = ">lllHHHH"

    packed = flatten(
        [
            struct.pack(
                struct_format, *to_fix16_vector(face_data.normal), *face_data.indices
            )
            for face_data in face_datas
        ]
    )

    return packed


def pack_attribute(attribute):
    if not isinstance(attribute, Attribute):
        raise TypeError("Expected Attribute, got %s" % (type(attribute)))

    # ATTRIBUTE -> (f, s, t, c, g, a, d, o)

    # ATTR::flag
    attr_flag = attribute.plane_type.value

    # ATTR::sort -> (s) | (((d) >> 16) & 0x1C) | (o)
    #               (o) is missing
    attr_sort = attribute.sort_type.value
    if (
        (attribute.primitive_type == PrimitiveType.NORMAL_SPRITE)
        or (attribute.primitive_type == PrimitiveType.SCALED_SPRITE)
        or (attribute.primitive_type == PrimitiveType.DISTORTED_SPRITE)
    ):
        # UseTexture (1 << 2)
        # UseLight   (1 << 3)
        # UsePalette (1 << 5)
        attr_sort |= 1 << 2

    # ATTR::texno
    attr_texno = attribute.texture_num

    # ATTR::atrb -> (a) | (((d) >> 24) & 0xC0)
    attr_atrb = attribute.attribute_flags.value
    if (
        (attribute.primitive_type == PrimitiveType.POLYGON)
        or (attribute.primitive_type == PrimitiveType.POLYLINE)
        or (attribute.primitive_type == PrimitiveType.LINE)
    ):
        attr_atrb |= int(AttributeFlags.SP_DISABLE | AttributeFlags.EC_DISABLE)

    # ATTR::colno
    attr_colno = 0xFFFF
    if attribute.color_data.color_type == ColorType.RGB:
        attr_colno = to_rgb1555(attribute.color_data.data)

    # ATTR::gstb
    attr_gstb = attribute.gouraud_table_num if attribute.face_data.colors else 0

    # ATTR::dir  -> (d) & 0x3F
    attr_dir = attribute.flip_dir.value | attribute.primitive_type.value

    # UseTexture  = (1 << 2)
    # UseLight    = (1 << 3)
    # UsePalette  = (1 << 5)
    #
    # sprHflip    = (1 << 4) | FUNC_Texture | (UseTexture << 16)
    # sprVflip    = (1 << 5) | FUNC_Texture | (UseTexture << 16)
    # sprHVflip   = (3 << 4) | FUNC_Texture | (UseTexture << 16)
    # sprNoflip   = (0)      | FUNC_Texture | (UseTexture << 16)
    # sprPolygon  = FUNC_Polygon  | ((ECdis | SPdis) << 24)
    # sprPolyLine = FUNC_PolyLine | ((ECdis | SPdis) << 24)
    # sprLine     = FUNC_Line     | ((ECdis | SPdis) << 24)

    struct_format = ">BBHHHHH"

    packed = struct.pack(
        struct_format,
        attr_flag,
        attr_sort,
        attr_texno,
        attr_atrb,
        attr_colno,
        attr_gstb,
        attr_dir,
    )

    # print("Attribute -> %s" % (packed))

    return packed


def pack_attributes(face_datas):
    return flatten([pack_attribute(face_data.attribute) for face_data in face_datas])


def pack_normals(face_datas):
    pass


def pack_xpdata(xpdata):
    if not isinstance(xpdata, XPDATA):
        raise TypeError("Expected XPDATA, got %s" % (type(xpdata)))

    packed_xpdata = PackedXPDATA()

    packed_xpdata.xpdata = xpdata
    packed_xpdata.packed_points = pack_points(xpdata.vertex_datas)
    packed_xpdata.packed_polygons = pack_polygons(xpdata.face_datas)
    packed_xpdata.packed_attributes = pack_attributes(xpdata.face_datas)
    # XXX: Fix me
    # normals_bytes = pack_normals(xpdata.face_datas)
    packed_xpdata.packed_normals = list(
        b"\x44" * (VECTOR_BYTE_SIZE * len(xpdata.face_datas))
    )
    packed_xpdata.packed_gouraud_tables = pack_gouraud_tables(xpdata.face_datas)

    return packed_xpdata


def pack_gouraud_tables(face_datas):
    packed_gouraud_tables = flatten(
        [
            map(
                to_rgb1555,
                [
                    face_data.colors[0],
                    face_data.colors[1],
                    face_data.colors[2],
                    face_data.colors[3],
                ],
            )
            for face_data in face_datas
            if face_data.colors
        ]
    )
    # We need to convert the 2-byte values to bytes
    return struct.pack((">%iH" % (len(packed_gouraud_tables))), *packed_gouraud_tables)


################################################################################
# Writing


def write_xpdata_headers(fp, s3d):
    if not isinstance(s3d, S3D):
        raise TypeError("Expected S3D, got %s" % (type(s3d)))

    points_offset = s3d.pools_offset
    polygons_offset = points_offset + s3d.total_points_size
    attributes_offset = polygons_offset + s3d.total_polygons_size
    normals_offset = attributes_offset + s3d.total_attributes_size

    for packed_xpdata in s3d.packed_xpdatas:
        points_count = len(packed_xpdata.xpdata.vertex_datas)
        points_size = points_count * VECTOR_BYTE_SIZE

        polygons_count = len(packed_xpdata.xpdata.face_datas)
        polygons_size = polygons_count * POLYGON_BYTE_SIZE

        attributes_count = polygons_count
        attributes_size = attributes_count * Attribute.BYTE_SIZE

        normals_count = polygons_count
        normals_size = VECTOR_BYTE_SIZE * normals_count

        packed_header = struct.pack(
            ">LLLLLL",
            points_offset,
            points_count,
            polygons_offset,
            polygons_count,
            attributes_offset,
            normals_offset,
        )

        fp.write(packed_header)

        points_offset += points_size
        polygons_offset += polygons_size
        attributes_offset += attributes_size
        normals_offset += normals_size


def write_aux_headers(fp, s3d):
    if not isinstance(s3d, S3D):
        raise TypeError("Expected S3D, got %s" % (type(s3d)))

    gouraud_tables_offset = (
        s3d.pools_offset
        + s3d.total_points_size
        + s3d.total_polygons_size
        + s3d.total_attributes_size
        + s3d.total_normals_size
    )

    cg_offset = 0x0000000

    for packed_xpdata in s3d.packed_xpdatas:
        gouraud_tables_size = len(packed_xpdata.packed_gouraud_tables)
        cg_size = 0x00000000

        packed_header = struct.pack(
            ">LHLL2x",
            0 if (gouraud_tables_size == 0) else gouraud_tables_offset,
            gouraud_tables_size,
            0 if (cg_size == 0) else cg_offset,
            cg_size,
        )

        gouraud_tables_offset += gouraud_tables_size

        fp.write(packed_header)


def write_picture_headers(fp, s3d):
    if not isinstance(s3d, S3D):
        raise TypeError("Expected S3D, got %s" % (type(s3d)))


def write_texture_headers(fp, s3d):
    if not isinstance(s3d, S3D):
        raise TypeError("Expected S3D, got %s" % (type(s3d)))


def write_pools(fp, s3d):
    if not isinstance(s3d, S3D):
        raise TypeError("Expected S3D, got %s" % (type(s3d)))

    for packed_xpdata in s3d.packed_xpdatas:
        fp.write(
            struct.pack(
                ("%iB" % len(packed_xpdata.packed_points)), *packed_xpdata.packed_points
            )
        )

    for packed_xpdata in s3d.packed_xpdatas:
        fp.write(
            struct.pack(
                ("%iB" % len(packed_xpdata.packed_polygons)),
                *packed_xpdata.packed_polygons
            )
        )

    for packed_xpdata in s3d.packed_xpdatas:
        fp.write(
            struct.pack(
                ("%iB" % len(packed_xpdata.packed_attributes)),
                *packed_xpdata.packed_attributes
            )
        )

    for packed_xpdata in s3d.packed_xpdatas:
        fp.write(
            struct.pack(
                ("%iB" % len(packed_xpdata.packed_normals)),
                *packed_xpdata.packed_normals
            )
        )

    for packed_xpdata in s3d.packed_xpdatas:
        fp.write(
            struct.pack(
                ("%iB" % len(packed_xpdata.packed_gouraud_tables)),
                *packed_xpdata.packed_gouraud_tables
            )
        )


class ExportSegaSaturnModel(bpy.types.Operator):
    """
    Export blender objects to Sega Saturn model.
    """

    bl_idname = "export.to_saturn"
    bl_label = "Export S3D (.S3D)"
    bl_options = {"REGISTER", "UNDO"}

    filepath: bpy.props.StringProperty(subtype="FILE_PATH")

    @classmethod
    def poll(cls, context):
        return bpy.context.selected_objects is not None

    def execute(self, context):
        filepath = pathlib.Path(self.filepath)

        return {"FINISHED"}

    def invoke(self, context, event):
        context.window_manager.fileselect_add(self)
        return {"RUNNING_MODAL"}


################################################################################
# Settings


class SaturnObjectSettings(bpy.types.PropertyGroup):
    # fmt: off
    # (identifier, name, description, icon, number)
    _ENUM_SORT_ITEMS = [
        ("SORT_BFR", "Before in front", "Display the face that was registered last in front.", "", 0),
        ("SORT_MIN", "Minimum", "Use the point in the face closest to the camera the reference point.", "", 1),
        ("SORT_MAX", "Maximum", "Use the point in the face furthest away from the camera the reference point.", "", 2),
        ("SORT_CEN", "Center", "Use the center point of the face as the reference point.", "", 3)
    ]

    _ENUM_PLANE_ITEMS = [
        ("SINGLE_PLANE", "Single", "Treat the face as single sided.", "", 0),
        ("DUAL_PLANE", "Dual", "Treat the face as double sided.", "", 1)
    ]

    _ENUM_ATTRIBUTE_HSS_ITEMS = [
        ("HSS_OFF", "Disable HSS", "Disable high speed shrink", "", 0),
        ("HSS_ON", "Enable HSS", "Enable high speed shrink", "", 1)
    ]

    _ENUM_ATTRIBUTE_MESH_ITEMS = [
        ("MESH_OFF", "Disable mesh", "Enable mesh display", "", 0),
        ("MESH_ON", "Enable mesh", "disable mesh display", "", 1)
    ]

    _ENUM_ATTRIBUTE_WINDOW_ITEMS = [
        ("NO_WINDOW", "No window", "", "", 0),
        ("WINDOW_IN", "Window in", "", "", 1),
        ("WINDOW_OUT", "Window out", "", "", 2)
    ]

    _ENUM_ATTRIBUTE_COLOR_MODE_ITEMS = [
        ("CL_16_BANK", "16 colors (color bank)", "", "", 0),
        ("CL_16_LOOK", "16 colors (LUT)", "", "", 1),
        ("CL_64_BANK", "64 colors (color bank)", "", "", 2),
        ("CL_128_BANK", "128 colors (color bank)", "", "", 3),
        ("CL_256_BANK", "256 colors (color bank)", "", "", 4),
        ("CL_32K_RGB", "32,768 colors (RGB)", "", "", 5)
    ]

    _ENUM_ATTRIBUTE_COLOR_CALCULATION_ITEMS = [
        ("CL_REPLACE", "Replace", "Mode 0", "", 0),
        ("CL_SHADOW", "Shadow", "Mode 1", "", 1),
        ("CL_HALF", "Half-luminance", "Mode 2", "", 2),
        ("CL_TRANS", "Half-transparent", "Mode 3", "", 3),
        ("CL_GOURAUD", "Gouraud shading", "Mode 4", "", 4),
        ("CL_GOURAUD+CL_HALF", "Gouraud+Half-luminance", "Mode 6", "", 5),
        ("CL_GOURAUD+CL_TRANS", "Gouraud+Half-transparent", "Mode 7", "", 6)
    ]
    # fmt: on

    plane: bpy.props.EnumProperty(
        name="plane",
        items=_ENUM_PLANE_ITEMS,
        description="Plane type",
        default="SINGLE_PLANE",
        update=None,
        get=None,
        set=None,
    )

    sort: bpy.props.EnumProperty(
        name="sort",
        items=_ENUM_SORT_ITEMS,
        description="Sorting",
        default="SORT_CEN",
        update=None,
        get=None,
        set=None,
    )

    attribute_hss: bpy.props.EnumProperty(
        name="attribute_hss",
        items=_ENUM_ATTRIBUTE_HSS_ITEMS,
        description="HSS",
        default="HSS_ON",
        update=None,
        get=None,
        set=None,
    )

    attribute_mesh: bpy.props.EnumProperty(
        name="attribute_mesh",
        items=_ENUM_ATTRIBUTE_MESH_ITEMS,
        description="Mesh",
        default="MESH_OFF",
        update=None,
        get=None,
        set=None,
    )

    attribute_window: bpy.props.EnumProperty(
        name="attribute_window",
        items=_ENUM_ATTRIBUTE_WINDOW_ITEMS,
        description="Windowing",
        default="NO_WINDOW",
        update=None,
        get=None,
        set=None,
    )

    attribute_flip_h: bpy.props.BoolProperty(
        name="attribute_flip_h", description="Flip texture horizontally"
    )

    attribute_flip_v: bpy.props.BoolProperty(
        name="attribute_flip_v", description="Flip texture vertically"
    )

    attribute_color_mode: bpy.props.EnumProperty(
        name="attribute_color_mode",
        items=_ENUM_ATTRIBUTE_COLOR_MODE_ITEMS,
        description="Color mode",
        default="CL_16_BANK",
        update=None,
        get=None,
        set=None,
    )

    attribute_color_calculation: bpy.props.EnumProperty(
        name="attribute_color_calculation",
        description="Color calculation",
        items=_ENUM_ATTRIBUTE_COLOR_CALCULATION_ITEMS,
        default="CL_REPLACE",
        update=None,
        get=None,
        set=None,
    )


################################################################################
# Panel


class FACE_ATTRIBUTE_PT_Panel(bpy.types.Panel):
    bl_label = "Sega Saturn face attributes"
    bl_region_type = "UI"
    bl_space_type = "VIEW_3D"

    @classmethod
    def poll(cls, context):
        # Only allow in edit mode for a selected mesh
        return (
            (context.mode == "EDIT_MESH")
            and context.object is not None
            and (context.object.type == "MESH")
        )

    def draw(self, context):
        layout = self.layout

        obj = context.object
        mesh = bmesh.from_edit_mesh(obj.data)

        selected_faces = get_selected_faces(mesh)
        if len(selected_faces) != 0:
            if len(selected_faces) > 1:
                layout.label(text="Multiple faces selected.")
                layout.separator()

            saturn_settings = obj.saturn_settings

            layout.label(text="Settings")
            box = layout.box()
            row = box.row()
            row.prop(saturn_settings, "plane", text="Plane")
            row = box.row()
            row.prop(saturn_settings, "sort", text="Sort")

            layout.separator()

            layout.label(text="Attributes")
            box = layout.box()
            row = box.row()
            row.prop(saturn_settings, "attribute_hss", text="HSS")
            row = box.row()
            row.prop(saturn_settings, "attribute_mesh", text="Mesh")
            row = box.row()
            row.prop(saturn_settings, "attribute_window", text="Windowing")

            layout.separator()

            layout.label(text="Texture settings")
            box = layout.box()
            row = box.row()
            row.prop(saturn_settings, "attribute_color_mode", text="Color mode")
            row = box.row()
            column = row.column()
            column.prop(saturn_settings, "attribute_flip_h", text="Flip horizontally")
            column = row.column()
            column.prop(saturn_settings, "attribute_flip_v", text="Flip vertically")
        else:
            layout.label(text="No face selected.")


# Only needed if you want to add into a dynamic menu
def menu_func(self, context):
    self.layout.operator_context = "INVOKE_DEFAULT"
    self.layout.operator(ExportSegaSaturnModel.bl_idname, text="Export to Sega Saturn")


_CLASSES = (ExportSegaSaturnModel, SaturnObjectSettings, FACE_ATTRIBUTE_PT_Panel)


def register():
    from bpy.utils import register_class

    for cls in _CLASSES:
        register_class(cls)

    bpy.types.TOPBAR_MT_file_export.append(menu_func)

    bpy.types.Object.saturn_settings = bpy.props.PointerProperty(
        name="Saturn object settings",
        description="Object Settings",
        type=SaturnObjectSettings,
    )

    # bpy.app.handlers.scene_update_post.clear()


def unregister():
    bpy.utils.unregister_class(ExportSegaSaturnModel)

    bpy.types.TOPBAR_MT_file_export.remove(menu_func)
    # bpy.app.handlers.depsgraph_update_post.clear()


# This allows you to run the script directly from blenders text editor to test
# the addon without having to install it.
if __name__ == "__main__":
    register()

    s3d = S3D()
    gouraud_table = GouraudTable()

    saturn_rot = bpy_extras.io_utils.axis_conversion(
        from_forward="Y", from_up="Z", to_forward="Z", to_up="-Y"
    ).to_4x4()

    for obj in bpy.context.selected_objects:
        _, obj_rot, obj_scale = obj.matrix_world.decompose()
        obj_rot = obj_rot.to_matrix().to_4x4()
        obj_scale = mathutils.Matrix.Diagonal(obj_scale).to_4x4()

        matrix = saturn_rot

        # XXX: Add a flag to apply rotation
        if True:
            matrix = matrix @ obj_rot

        # XXX: Add a flag to apply scale
        if False:
            matrix = matrix @ obj_scale

        mesh = bmesh.new()
        mesh.from_mesh(obj.data)
        bmesh.ops.transform(mesh, matrix=matrix, verts=mesh.verts)

        xpdata = XPDATA()
        xpdata.vertex_datas = extract_vertices(mesh)
        xpdata.face_datas = extract_faces(gouraud_table, obj, mesh)
        packed_xpdata = pack_xpdata(xpdata)
        s3d.packed_xpdatas.append(packed_xpdata)

    output_path = "C:/msys64/work/libyaul/repositories/private/libyaul-examples-private/vdp1-sega3d/romdisk/XPDATA.DAT"

    with open(output_path, "wb+") as fp:
        xpdata_count = len(s3d.packed_xpdatas)
        picture_count = len(s3d.packed_pictures)
        pool_size = (
            s3d.total_points_size
            + s3d.total_polygons_size
            + s3d.total_attributes_size
            + s3d.total_normals_size
        )

        fp.write(struct.pack(">3sB", b"S3D", 0))
        fp.write(struct.pack(">H", S3D.VERSION))
        fp.write(struct.pack(">L", s3d.flags.value))
        fp.write(struct.pack(">H", xpdata_count))
        fp.write(struct.pack(">H", picture_count))
        diff = S3D.BYTE_SIZE - fp.tell()
        # Pad out the header to 64 bytes
        fp.write(struct.pack("%ix" % (diff)))

        write_xpdata_headers(fp, s3d)
        write_aux_headers(fp, s3d)
        write_picture_headers(fp, s3d)
        write_texture_headers(fp, s3d)
        write_pools(fp, s3d)
