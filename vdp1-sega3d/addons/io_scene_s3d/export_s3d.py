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

import bpy, bmesh, subprocess, collections, re
from bpy import ops
from bpy.app.translations import pgettext
from mathutils import *
from math import *
from bpy.types import Collection
from bpy.props import CollectionProperty, StringProperty, BoolProperty

from .utilities import *


class S3DExporter(bpy.types.Operator, Logger):
    get_id("exporter_tip")
    bl_idname = "export_scene.smd"
    bl_label = get_id("exporter_title")

    export_scene: bpy.props.BoolProperty(
        name=get_id("scene_export"),
        description=get_id("exporter_prop_scene_tip"),
        default=False,
    )

    @classmethod
    def poll(cls, context):
        return len(context.scene.s3d.export_list)

    def invoke(self, context, event):
        # scene_update(context.scene, immediate=True)
        ops.wm.call_menu(name="S3D_MT_ExportChoice")
        return {"PASS_THROUGH"}

    def execute(self, context):
        if not context.scene.s3d.export_path:
            bpy.ops.wm.call_menu(name="S3D_MT_ConfigureScene")
            return {"CANCELLED"}
        if (
            context.scene.s3d.export_path.startswith("//")
            and not context.blend_data.filepath
        ):
            self.report({"ERROR"}, get_id("exporter_err_relativeunsaved"))
            return {"CANCELLED"}
        if self.export_scene:
            print("Executing")
        self.export_scene = False
        return {"FINISHED"}
