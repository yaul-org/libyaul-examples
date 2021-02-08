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

import bpy
import bmesh
import subprocess
import collections
import re
from bpy import ops
from bpy.app.translations import pgettext
from mathutils import *
from math import *
from bpy.types import Collection
from bpy.props import CollectionProperty, StringProperty, BoolProperty

from .utilities import *


class S3DExporter(bpy.types.Operator, Logger):
    get_id("exporter_tip")
    bl_idname = "export_scene.s3d"
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
        scene_update(context.scene, immediate=True)
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

        # Don't create an undo level from edit mode
        prev_mode = prev_hidden = None
        if context.active_object:
            if context.active_object.hide_viewport:
                prev_hidden = context.active_object
                context.active_object.hide_viewport = False
            prev_mode = context.mode
            if prev_mode.find("EDIT") != -1:
                prev_mode = "EDIT"
            elif prev_mode.find("PAINT") != -1:  # FFS Blender!
                prev_mode = prev_mode.split("_")
                prev_mode.reverse()
                prev_mode = "_".join(prev_mode)
            ops.object.mode_set(mode="OBJECT")

        scene_update(context.scene, immediate=True)

        ops.ed.undo_push(message=self.bl_label)

        try:
            if self.export_scene:
                for id in [
                    exportable.get_id() for exportable in context.scene.s3d.export_list
                ]:
                    if type(id) == Collection:
                        if should_export_group(id):
                            self.export_id(context, id)
                    elif id.s3d.export:
                        self.export_id(context, id)

        except:
            # Clean everything up
            ops.ed.undo_push(message=self.bl_label)
            if bpy.app.debug_value <= 1:
                ops.ed.undo()

            if prev_mode:
                ops.object.mode_set(mode=prev_mode)
            if prev_hidden:
                prev_hidden.hide_viewport = True
            bpy.context.scene.update_tag()

            bpy.context.window_manager.progress_end()
            hook_scene_update()

        self.export_scene = False

        return {"FINISHED"}

    def export_id(self, context, id):
        subdir = id.s3d.subdir
        subdir = subdir.lstrip("/")  # Don't want //s here!
        path = os.path.join(bpy.path.abspath(context.scene.s3d.export_path), subdir)

        print(
            '\nBlender SEGA3D Tools: Exporting {} to "{}" (export path: "{}")'.format(
                id.name, path, context.scene.s3d.export_path
            )
        )

        if not os.path.exists(path):
            try:
                os.makedirs(path)
            except Exception as err:
                self.error(get_id("exporter_err_makedirs", True).format(err))
                return

        if isinstance(id, Collection) and not any(ob.s3d.export for ob in id.objects):
            self.error(get_id("exporter_err_nogroupitems", True).format(id.name))
            return

        if type(id) == Collection:
            for i, ob in enumerate(
                [
                    ob
                    for ob in id.objects
                    if ob.s3d.export and ob in g_cache.valid_objects
                ]
            ):
                print("For loop collection: {}, {}".format(i, ob))

    def sanitize_filename(self, name):
        new_name = name
        for bad_char in '/?<>\\:*|"':
            new_name = new_name.replace(bad_char, "_")
            if new_name != name:
                self.warning(
                    get_id("exporter_warn_sanitised_filename", True).format(
                        name, new_name
                    )
                )
        return new_name
