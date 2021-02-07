import bpy
from .utilities import *
from .export_s3d import S3DExporter


class S3D_MT_ExportChoice(bpy.types.Menu):
    bl_label = get_id("exportmenu_title")

    def draw(self, context):
        l = self.layout
        l.operator_context = "EXEC_DEFAULT"
        row = l.row()
        export_count = count_exports(context)
        row.operator(
            S3DExporter.bl_idname,
            text=get_id("exportmenu_scene", True).format(export_count),
            icon="SCENE_DATA",
        ).export_scene = True
        row.enabled = export_count > 0


class S3D_MT_ConfigureScene(bpy.types.Menu):
    bl_label = "S3D Tools Error Report"

    def draw(self, context):
        self.layout.label(text=get_id("exporter_err_unconfigured"))
        S3D_PT_Scene.HelpButton(self.layout)


class S3D_PT_Scene(bpy.types.Panel):
    bl_label = get_id("exportpanel_title")
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "scene"
    bl_default_closed = True

    def draw(self, context):
        l = self.layout
        scene = context.scene
        num_to_export = 0

        l.operator(S3DExporter.bl_idname, text="Export")

        row = l.row()
        row.alert = len(scene.s3d.export_path) == 0
        row.prop(scene.s3d, "export_path")

        col = l.column(align=True)
        row = col.row(align=True)
        self.HelpButton(row)

    @staticmethod
    def HelpButton(layout):
        layout.operator(
            "wm.url_open", text=get_id("help", True), icon="HELP"
        ).url = "http://github.com/ijacquez/libyaul"
