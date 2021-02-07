import bpy
from .utilities import *
from .export_s3d import S3DExporter

global p_cache


class S3D_MT_ExportChoice(bpy.types.Menu):
    bl_label = "Export Label Goes HEre"

    def draw(self, context):
        pass
