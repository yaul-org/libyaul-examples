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

import os
import bpy
from bpy import ops
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

from . import export_s3d, gui
from .utilities import *


def menu_func_export(self, context):
    self.layout.menu("S3D_MT_ExportChoice", text="Sega Saturn (.s3d)")


classes = (export_s3d.S3DExporter, gui.S3D_MT_ExportChoice)


def register():
    for cls in classes:
        bpy.utils.register_class(cls)

    bpy.types.TOPBAR_MT_file_export.append(menu_func_export)
    # hook_scene_update()


def unregister():
    # unhook_scene_update()
    for cls in reversed(classes):
        bpy.utils.unregister_class(cls)

    bpy.types.TOPBAR_MT_file_export.remove(menu_func_export)


if __name__ == "__main__":
    register()
