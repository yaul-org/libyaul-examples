# Copyright (c) 2014 Tom Edwards <contact@steamreview.org>
# Copyright (c) 2021 Israel Jacquez <mrkotfw@gmail.com>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

# fmt: off
bl_info = {
    "name":        "Sega Saturn 3D Tools (.s3d)",
    "author":      "Israel Jacquez",
    "version":     (1, 0, 0),
    "blender":     (2, 80, 0),
    "location":    "File > Import/Export",
    "description": "Exporter for Sega Saturn 3D (libsega3d and libyaul).",
    "warning":     "",
    "wiki_url":    "https://github.com/ijacquez/libyaul",
    "tracker_url": "https://github.com/ijacquez/libyaul/issues",
    "category":    "Import-Export"
}
# fmt: on


def reload_package(module_dict_main):
    import importlib
    from pathlib import Path

    def reload_package_recursive(current_dir, module_dict):
        for path in current_dir.iterdir():
            if "__init__" in str(path) or path.stem not in module_dict:
                continue

            if path.is_file() and path.suffix == ".py":
                importlib.reload(module_dict[path.stem])
            elif path.is_dir():
                reload_package_recursive(path, module_dict[path.stem].__dict__)

    reload_package_recursive(Path(__file__).parent, module_dict_main)


if "bpy" in locals():
    reload_package(locals())

# clear out any scene update funcs hanging around, e.g. after a script reload
from bpy.app.handlers import depsgraph_update_pre, depsgraph_update_post

for func in depsgraph_update_post:
    if func.__module__.startswith(__name__):
        depsgraph_update_post.remove(func)

import os
import bpy
from bpy import ops
from bpy.types import PropertyGroup
from bpy.props import (
    StringProperty,
    BoolProperty,
    EnumProperty,
    IntProperty,
    CollectionProperty,
    FloatProperty,
    PointerProperty,
)
from bpy_extras.io_utils import ImportHelper, ExportHelper

from . import export_s3d, gui, translations
from .utilities import *


class S3D_Exportable(PropertyGroup):
    ob_type: StringProperty()
    icon: StringProperty()
    item_name: StringProperty()

    def get_id(self):
        try:
            if self.ob_type == "COLLECTION":
                return bpy.data.collections[self.item_name]
            if self.ob_type in ["ACTION", "OBJECT"]:
                return bpy.data.objects[self.item_name]
            else:
                raise TypeError(
                    'Unknown object type "{}" in S3D_Exportable'.format(self.ob_type)
                )
        except KeyError:
            bpy.context.scene.update_tag()


class S3D_SceneProps(PropertyGroup):
    export_path: StringProperty(
        name=get_id("exportroot"),
        description=get_id("exportroot_tip"),
        subtype="DIR_PATH",
    )
    export_list: CollectionProperty(
        type=S3D_Exportable, options={"SKIP_SAVE", "HIDDEN"}
    )


class ExportableProps:
    export: BoolProperty(
        name=get_id("scene_export"),
        description=get_id("use_scene_export_tip"),
        default=True,
    )
    subdir: StringProperty(name=get_id("subdir"), description=get_id("subdir_tip"))


class S3D_ObjectProps(ExportableProps, PropertyGroup):
    pass


def menu_func_export(self, context):
    self.layout.menu("S3D_MT_ExportChoice", text=get_id("export_menuitem"))


@bpy.app.handlers.persistent
def scene_load_post(_):
    def convert(id, *prop_groups):
        prop_map = {"export_path": "path"}

        for p_g in prop_groups:
            for prop in [prop for prop in p_g.__dict__.keys() if prop[0] != "_"]:
                val = id.get("s3d_" + (prop_map[prop] if prop in prop_map else prop))
                if val != None:
                    id.s3d[prop] = val

        for prop in id.keys():
            if prop.startswith("s3d_"):
                del id[prop]

    for s in bpy.data.scenes:
        if hasattr(s, "s3d"):
            convert(s, S3D_SceneProps)
            # game_path_changed(s, bpy.context)
            # engine_path_changed(s, bpy.context)
    for ob in bpy.data.objects:
        convert(ob, S3D_ObjectProps, ExportableProps)

    if scene_load_post in depsgraph_update_post:
        depsgraph_update_post.remove(scene_load_post)


classes = (
    S3D_Exportable,
    S3D_SceneProps,
    S3D_ObjectProps,
    export_s3d.S3DExporter,
    gui.S3D_MT_ExportChoice,
    gui.S3D_PT_Scene,
    gui.S3D_MT_ConfigureScene,
)


def register():
    for cls in classes:
        bpy.utils.register_class(cls)

    bpy.types.TOPBAR_MT_file_export.append(menu_func_export)

    hook_scene_update()

    bpy.app.handlers.load_post.append(scene_load_post)
    depsgraph_update_post.append(
        scene_load_post
    )  # Handles enabling the add-on after the scene is loaded

    def make_pointer(prop_type):
        return PointerProperty(name=get_id("settings_prop"), type=prop_type)

    bpy.types.Scene.s3d = make_pointer(S3D_SceneProps)
    bpy.types.Object.s3d = make_pointer(S3D_ObjectProps)


def unregister():
    unhook_scene_update()
    bpy.app.handlers.load_post.remove(scene_load_post)

    for cls in reversed(classes):
        bpy.utils.unregister_class(cls)

    bpy.types.TOPBAR_MT_file_export.remove(menu_func_export)

    del bpy.types.Scene.s3d
    del bpy.types.Object.s3d


if __name__ == "__main__":
    register()
