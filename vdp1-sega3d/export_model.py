# 0000  4B S3D '\0'
# 0004  2B Version number
# 0006  4B Flags
# 0010  2B XPDATA count                           XPDATA will always be at offset 64
# 0012  2B (Picture) Texture list count
# 0014  4B Texture file offset                    [0 if no texs] This gives us the ability to know how big the total XPDATA size is
# 0018  4B Texture byte size
# 0022  4B Palette file offset                    [0 if no pals]
# 0026  4B Palette byte size
# 0030  4B Gouraud table file offset
# 0034  2B Gouraud table count
# 0036 12B Origin (XYZ)
# 0048 12B BB length (XYZ)                        <---------- May be a good idea to have a list for each XPDATA (or just one)
# 0060  4B User offset to data
# 0064 XPDATA 1 - Each has offsets to their data
# .... XPDATA 2
# .... XPDATA 3
#
#
# PICTURE uses relative offsets to TEXTURE, which then points to actual texture offsets.
#  If the PICTURE has a palette type, then the palette

################################################################################
# 1. ATTRs are broken?
# 2. Triangle normals are broken
# 3. ColorData issue
# 4. Gouraud table support


import ctypes
import math
import os
import struct
import sys

from enum import Enum, IntFlag

import bpy
import bmesh
import mathutils
import pathlib
import gpu

from bpy.app.handlers import persistent

bl_info = {"name": "Export Sega Saturn Model (.ssm)", "category": "Import-Export"}
################################################################################
VECTOR_BYTE_SIZE = 4 * 3
POLYGON_BYTE_SIZE = VECTOR_BYTE_SIZE + (4 * 2)


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
        self.indices = [-1] * 4
        self.normal = mathutils.Vector((0.0, 0.0, 1.0))
        self.colors = [mathutils.Color((0.0, 0.0, 0.0))] * 4
        self.attribute = Attribute()


class VertexData(object):
    def __init__(self):
        self.point = mathutils.Vector((0.0, 0.0, 0.0))


class ColorData(object):
    def __init__(self):
        self.color_type = ColorType.RGB
        self.data = mathutils.Color((0.5, 0.5, 0.5))


class Attribute(object):
    BYTE_SIZE = 1 + 1 + (2 * 5)

    def __init__(self):
        self.plane_type = PlaneType.SINGLE_PLANE
        self.sort_type = SortType.SORT_CEN
        self.texture_num = 0
        self.color_data = ColorData()
        self.gouraud_table_num = 0
        self.attribute_flags = AttributeFlags.MESH_OFF
        self.primitive_type = PrimitiveType.POLYGON
        self.flip_dir = FlipDir.NONE


class XPDATA(object):
    BYTE_SIZE = 6 * 4

    def __init__(self):
        self.vertex_datas = []
        self.face_datas = []


################################################################################


class PackedXPDATA(object):
    def __init__(self):
        self.xpdata = XPDATA()
        self.packed_points = b""
        self.packed_polygons = b""
        self.packed_attributes = b""
        self.packed_normals = b""


################################################################################

# Utility
def to_fix16_vector(vector):
    if not isinstance(vector, mathutils.Vector):
        raise TypeError("Expected mathutils.Vector, got %s", (type(vector)))
    return tuple([to_fix16(component) for component in vector])


# Utility
def to_rgb1555(color):
    def clamp_color_component(value):
        return min(value, 1.0)

    r = ctypes.c_uint8(int(clamp_color_component(color[0]) * 255))
    g = ctypes.c_uint8(int(clamp_color_component(color[1]) * 255))
    b = ctypes.c_uint8(int(clamp_color_component(color[2]) * 255))

    byte_r = (r.value >> 3) & 0x1F
    byte_g = (g.value >> 3) & 0x1F
    byte_b = (b.value >> 3) & 0x1F

    return ctypes.c_uint16(0x8000 | (byte_b << 10) | (byte_g << 5) | (byte_r)).value


# Utility
def to_fix16(value):
    return int((value * 65536.0) + (0.5 if (value >= 0) else -0.5))


def flatten(lsts):
    return [item for sublist in lsts for item in sublist]


################################################################################


def extract_faces(obj, mesh):
    if not isinstance(obj, bpy.types.Object):
        raise TypeError("Expected bpy.types.Object, got %s" % type(obj))
    if not isinstance(mesh, bmesh.types.BMesh):
        raise TypeError("Expected bmesh.types.BMesh, got %s" % type(mesh))

    mesh.faces.ensure_lookup_table()
    mesh.verts.ensure_lookup_table()

    face_datas = []

    for face in mesh.faces:
        indices = []
        if len(face.loops) == 3:
            indices = [loop.index for loop in face.loops[0:3]] + [face.loops[0].index]
        elif len(face.loops) == 4:
            indices = [loop.index for loop in face.loops[0:4]]
        face_data = FaceData()
        face_data.indices = [obj.data.loops[index].vertex_index for index in indices]
        face_data.normal = face.normal.copy()

        # Perform a test to see if the winding is correct
        vertices = [mesh.verts[index].co for index in face_data.indices]

        u1 = (vertices[1] - vertices[0]).normalized()
        v1 = (vertices[3] - vertices[0]).normalized()
        n1 = u1.cross(v1)
        dir1 = face_data.normal.dot(n1)

        if not math.isclose(dir1, 1.0, abs_tol=0.001):
            pass
            # print("%s -> %s" % (face_data.normal, n1))

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


def pack_points(vertex_datas):
    points_bytes = flatten(
        [to_fix16_vector(vertex_data.point) for vertex_data in vertex_datas]
    )

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
    attr_gstb = attribute.gouraud_table_num

    # ATTR::dir  -> (d) & 0x3F
    attr_dir = attribute.flip_dir.value | attribute.primitive_type.value

    # UseTexture      (1 << 2)
    # UseLight        (1 << 3)
    # UsePalette      (1 << 5)
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

    print("Attribute -> %s" % (packed))

    return packed


def pack_attributes(face_datas):
    return flatten([pack_attribute(face_data.attribute) for face_data in face_datas])


def pack_normals(face_datas):
    pass


def pack_xpdata(xpdata):
    if not isinstance(xpdata, XPDATA):
        raise TypeError("Expected XPDATA, got %s" % (type(xpdata)))

    packed_xpdata = PackedXPDATA()

    # normals_bytes = pack_normals(xpdata.face_datas)
    packed_xpdata.xpdata = xpdata
    packed_xpdata.packed_points = pack_points(xpdata.vertex_datas)
    packed_xpdata.packed_polygons = pack_polygons(xpdata.face_datas)
    packed_xpdata.packed_attributes = pack_attributes(xpdata.face_datas)
    packed_xpdata.packed_normals = list(
        b"\x44" * (VECTOR_BYTE_SIZE * len(xpdata.face_datas))
    )

    return packed_xpdata


class ExportSegaSaturnModel(bpy.types.Operator):
    """Export blender objects to Sega Saturn model"""

    bl_idname = "export.to_saturn"
    bl_label = "Export Saturn Model (.SSM)"

    filepath: bpy.props.StringProperty(subtype="FILE_PATH")

    checkForDuplicatedTextures: bpy.props.BoolProperty(
        name="Check for duplicated textures",
        description="Check for duplicated textures",
        default=True,
    )

    useAO: bpy.props.BoolProperty(
        name="Use AO", description="Use secondary texture/uv as AO", default=False
    )

    texturesSizeByArea: bpy.props.BoolProperty(
        name="Use area to discover texture size",
        description="Use area to discover texture size",
        default=True,
    )

    minimumTextureSize: bpy.props.IntProperty(
        name="Minimum Texture size",
        description="Minimum texture size",
        default=8,
        max=128,
        min=2,
        step=2,
    )

    # Resolution of output textures.
    outputTextureSize: bpy.props.IntProperty(
        name="Texture Size (size x size)",
        description="Texture Size (size x size)",
        default=16,
        max=128,
        min=2,
        step=2,
    )

    @classmethod
    def poll(cls, context):
        return bpy.context.selected_objects is not None

    def execute(self, context):
        filepath = pathlib.Path(self.filepath)
        with filepath.open("wb") as out_fp:
            pass

        return {"FINISHED"}

    def invoke(self, context, event):
        context.window_manager.fileselect_add(self)
        return {"RUNNING_MODAL"}


# Only needed if you want to add into a dynamic menu
def menu_func(self, context):
    self.layout.operator_context = "INVOKE_DEFAULT"
    self.layout.operator(ExportSegaSaturnModel.bl_idname, text="Export to Sega Saturn")


def getActiveFaces(obj):
    faces = []
    for face in obj.faces:
        if face.select:
            faces.append(face)

    return faces


def register():
    bpy.utils.register_class(ExportSegaSaturnModel)
    bpy.types.TOPBAR_MT_file_export.append(menu_func)


def unregister():
    bpy.utils.unregister_class(ExportSegaSaturnModel)

    bpy.types.TOPBAR_MT_file_export.remove(menu_func)
    bpy.app.handlers.depsgraph_update_post.clear()


# This allows you to run the script directly from blenders text editor to test
# the addon without having to install it.
if __name__ == "__main__":
    register()

    packed_xpdatas = []

    for obj in bpy.context.selected_objects:
        mesh = bmesh.new()
        mesh.from_mesh(obj.data)

        xpdata = XPDATA()
        xpdata.vertex_datas = extract_vertices(mesh)
        xpdata.face_datas = extract_faces(obj, mesh)
        packed_xpdata = pack_xpdata(xpdata)
        packed_xpdatas.append(packed_xpdata)

    output_path = "C:/msys64/work/libyaul/repositories/private/libyaul-examples-private/vdp1-sega3d/romdisk/XPDATA.DAT"

    with open(output_path, "wb+") as fp:
        total_points_size = sum(
            [len(packed_xpdata.packed_points) for packed_xpdata in packed_xpdatas]
        )
        total_polygons_size = sum(
            [len(packed_xpdata.packed_polygons) for packed_xpdata in packed_xpdatas]
        )
        total_attributes_size = sum(
            [len(packed_xpdata.packed_attributes) for packed_xpdata in packed_xpdatas]
        )
        total_normals_size = sum(
            [len(packed_xpdata.packed_normals) for packed_xpdata in packed_xpdatas]
        )

        total_xpdatas_size = len(packed_xpdatas) * XPDATA.BYTE_SIZE

        points_offset = total_xpdatas_size
        polygons_offset = total_xpdatas_size + total_points_size
        attributes_offset = polygons_offset + total_polygons_size
        normals_offset = attributes_offset + total_attributes_size

        for packed_xpdata in packed_xpdatas:
            # XPDATA -> 24B
            # POINT   *pntbl      4B offset
            # Uint32   nbPoint    4B
            # POLYGON *pltbl      4B offset
            # Uint32   nbPolygon  4B
            # ATTR    *attbl      4B offset
            # VECTOR  *vntbl      4B offset

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

        for packed_xpdata in packed_xpdatas:
            fp.write(
                struct.pack(
                    ("%iB" % len(packed_xpdata.packed_points)),
                    *packed_xpdata.packed_points
                )
            )

        for packed_xpdata in packed_xpdatas:
            fp.write(
                struct.pack(
                    ("%iB" % len(packed_xpdata.packed_polygons)),
                    *packed_xpdata.packed_polygons
                )
            )

        for packed_xpdata in packed_xpdatas:
            fp.write(
                struct.pack(
                    ("%iB" % len(packed_xpdata.packed_attributes)),
                    *packed_xpdata.packed_attributes
                )
            )

        for packed_xpdata in packed_xpdatas:
            fp.write(
                struct.pack(
                    ("%iB" % len(packed_xpdata.packed_normals)),
                    *packed_xpdata.packed_normals
                )
            )
