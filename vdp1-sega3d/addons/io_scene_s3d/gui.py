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
    bl_label = get_id("exporter_report_menu")

    def draw(self, context):
        self.layout.label(text=get_id("exporter_err_unconfigured"))
        S3D_PT_Scene.HelpButton(self.layout)


class S3D_UL_GroupItems(bpy.types.UIList):
    def draw_item(
        self, context, layout, data, item, icon, active_data, active_propname, index
    ):
        r = layout.row(align=True)
        r.prop(
            item.s3d,
            "export",
            text="",
            icon="CHECKBOX_HLT" if item.s3d.export else "CHECKBOX_DEHLT",
            emboss=False,
        )
        r.label(
            text=item.name, translate=False, icon=make_object_icon(item, suffix="_DATA")
        )

    def filter_items(self, context, data, propname):
        fname = self.filter_name.lower()

        filtered = [
            (
                self.bitflag_filter_item
                if ob in g_cache.valid_objects
                and (not fname or (fname in ob.name.lower()))
                else 0
            )
            for ob in data.objects
        ]
        ordered = bpy.types.UI_UL_list.sort_items_by_name(data.objects)

        return filtered, ordered


class S3D_PT_Object_Config(bpy.types.Panel):
    bl_label = get_id("exportables_title")
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "scene"
    bl_default_closed = True

    def draw(self, context):
        l = self.layout
        scene = context.scene

        l.template_list(
            "S3D_UL_ExportItems",
            "",
            scene.s3d,
            "export_list",
            scene.s3d,
            "export_list_active",
            rows=3,
            maxrows=8,
        )

        active_exportable = get_active_exportable(context)
        if not active_exportable:
            return

        item = active_exportable.get_id()
        is_group = type(item) == bpy.types.Collection

        if not (is_group and item.s3d.mute):
            l.column().prop(item.s3d, "subdir", icon="FILE_FOLDER")


class ExportableConfigurationPanel(bpy.types.Panel):
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "scene"
    bl_parent_id = "S3D_PT_Object_Config"
    s3d_icon = ""

    @classmethod
    def get_item(cls, context):
        active_exportable = get_active_exportable(context)
        if not active_exportable:
            return

        return active_exportable.get_id()

    @classmethod
    def poll(cls, context):
        return cls.get_item(context) is not None

    @classmethod
    def is_collection(cls, item):
        return isinstance(item, bpy.types.Collection)

    @classmethod
    def get_active_object(cls, context):
        item = cls.get_item(context)

        if not cls.is_collection(item):
            return item

        ob = context.active_object
        if ob and (ob.type == "MESH") and ob.name in item.objects:
            return ob

    @classmethod
    def unpack_collection(cls, context):
        item = cls.get_item(context)
        return (
            g_cache.valid_objects.intersection(item.objects)
            if cls.is_collection(item)
            else [item]
        )

    def draw_header(self, context):
        if self.s3d_icon:
            self.layout.label(icon=self.s3d_icon)


class S3D_PT_Group(ExportableConfigurationPanel):
    bl_label = get_id("exportables_group_props")
    s3d_icon = "GROUP"

    @classmethod
    def poll(cls, context):
        item = cls.get_item(context)
        return item and cls.is_collection(item)

    def draw(self, context):
        item = self.get_item(context)
        if not item.s3d.mute:
            self.layout.template_list(
                "S3D_UL_GroupItems",
                item.name,
                item,
                "objects",
                item.s3d,
                "selected_item",
                type="GRID",
                columns=2,
                rows=2,
                maxrows=10,
            )

        r = self.layout.row()
        r.alignment = "CENTER"
        r.prop(item.s3d, "mute")
        if item.s3d.mute:
            return
        else:
            return
            # r.prop(item.s3d, "automerge")


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


class S3D_UL_ExportItems(bpy.types.UIList):
    def draw_item(
        self, context, layout, data, item, icon, active_data, active_propname, index
    ):
        id = item.get_id()
        if id is None:
            return

        enabled = not (type(id) == bpy.types.Collection and id.s3d.mute)

        row = layout.row(align=True)
        row.alignment = "LEFT"
        row.enabled = enabled

        row.prop(
            id.s3d,
            "export",
            icon="CHECKBOX_HLT" if id.s3d.export and enabled else "CHECKBOX_DEHLT",
            text="",
            emboss=False,
        )
        row.label(text=item.name, icon=item.icon)

        if not enabled:
            return
