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
import struct
import time
import collections
import os
import subprocess
import sys
import builtins
import itertools

from bpy.app.handlers import depsgraph_update_post, persistent
from bpy.app.translations import pgettext
from mathutils import Matrix, Vector, Euler, Quaternion
from math import *

# fmt: off

_G_EXPORTABLE_TYPES = (
    'MESH'
)
# fmt: on


class Cache:
    valid_objects = set()
    valid_objects_version = 0

    from . import translations

    ids = translations.ids

    @classmethod
    def __del__(cls):
        cls.valid_objects.clear()


global g_cache
g_cache = globals().get("g_cache", Cache())

# Local
_g_last_export_refresh = 0


def get_id(id, format_string=False, data=False):
    from . import translations

    ids = translations.ids

    return ids[id]


def count_exports(context):
    count = 0
    for exportable in context.scene.s3d.export_list:
        id = exportable.get_id()
        if (
            id
            and id.s3d.export
            and (type(id) != bpy.types.Collection or not id.s3d.mute)
        ):
            count += 1
    return count


def get_file_extension():
    return ".s3d"


def get_active_exportable(context=None):
    if not context:
        context = bpy.context

    if not context.scene.s3d.export_list_active < len(context.scene.s3d.export_list):
        return None

    return context.scene.s3d.export_list[context.scene.s3d.export_list_active]


def get_exportables_for_id(id):
    if not id:
        raise ValueError("id is null")
    out = set()
    for exportable in bpy.context.scene.s3d.export_list:
        if exportable.get_id() == id:
            return [exportable]
        if exportable.ob_type == "COLLECTION":
            collection = exportable.get_id()
            if not collection.s3d.mute and id.name in collection.objects:
                out.add(exportable)
    return list(out)


def get_selected_exportables():
    exportables = set()
    for ob in bpy.context.selected_objects:
        exportables.update(get_exportables_for_id(ob))
    if (len(exportables) == 0) and bpy.context.active_object:
        a_e = get_exportables_for_id(bpy.context.active_object)
        if a_e:
            exportables.update(a_e)
    return exportables


def should_export_group(group):
    return group.s3d.export and not group.s3d.mute


def make_object_icon(object, prefix=None, suffix=None):
    if not (prefix or suffix):
        raise TypeError("A prefix or suffix is required")

    type = object.type

    out = ""
    if prefix:
        out += prefix
    out += type
    if suffix:
        out += suffix
    return out


def _make_export_list():
    s = bpy.context.scene
    s.s3d.export_list.clear()

    def make_display_name(item, name=None):
        return os.path.join(
            item.s3d.subdir if item.s3d.subdir != "." else "",
            (name if name else item.name) + get_file_extension(),
        )

    if len(g_cache.valid_objects):
        ungrouped_objects = g_cache.valid_objects.copy()

        groups_sorted = bpy.data.collections[:]
        groups_sorted.sort(key=lambda g: g.name.lower())

        scene_groups = []
        for group in groups_sorted:
            valid = False
            for object in [ob for ob in group.objects if ob in g_cache.valid_objects]:
                if not group.s3d.mute and object in ungrouped_objects:
                    ungrouped_objects.remove(object)
                valid = True
            if valid:
                scene_groups.append(group)

        for g in scene_groups:
            i = s.s3d.export_list.add()
            if g.s3d.mute:
                i.name = "{} {}".format(
                    g.name, pgettext(get_id("exportables_group_mute_suffix", True))
                )
            else:
                i.name = make_display_name(g)
            i.item_name = g.name
            i.ob_type = "COLLECTION"
            i.icon = "GROUP"

        ungrouped_objects = list(ungrouped_objects)
        ungrouped_objects.sort(key=lambda ob: ob.name.lower())
        for ob in ungrouped_objects:
            i_name = i_type = i_icon = None
            if ob.type == "MESH":
                i_name = make_display_name(ob)
                i_icon = make_object_icon(ob, prefix="OUTLINER_OB_")
                i_type = "OBJECT"
            if i_name:
                i = s.s3d.export_list.add()
                i.name = i_name
                i.ob_type = i_type
                i.icon = i_icon
                i.item_name = ob.name


@persistent
def scene_update(scene, immediate=False):
    global _g_last_export_refresh

    if not hasattr(scene, "s3d"):
        return

    g_cache.valid_objects = set(
        [ob for ob in scene.objects if ob.type in _G_EXPORTABLE_TYPES]
    )

    now = time.time()

    if immediate or now - _g_last_export_refresh > 0.125:
        _make_export_list()
        _g_last_export_refresh = now


def hook_scene_update():
    if not scene_update in depsgraph_update_post:
        depsgraph_update_post.append(scene_update)


def unhook_scene_update():
    if scene_update in depsgraph_update_post:
        depsgraph_update_post.remove(scene_update)


class Logger:
    def __init__(self):
        self.log_warnings = []
        self.log_errors = []
        self.start_time = time.time()

    def warning(self, *string):
        message = " ".join(str(s) for s in string)
        print(" WARNING:", message)
        self.log_warnings.append(message)

    def error(self, *string):
        message = " ".join(str(s) for s in string)
        print(" ERROR:", message)
        self.log_errors.append(message)

    def list_errors(self, menu, context):
        l = menu.layout
        if len(self.log_errors):
            for msg in self.log_errors:
                l.label(text="{}: {}".format(pgettext("Error").upper(), msg))
            l.separator()
        if len(self.log_warnings):
            for msg in self.log_warnings:
                l.label(text="{}: {}".format(pgettext("Warning").upper(), msg))

    def elapsed_time(self):
        return round(time.time() - self.start_time, 1)

    def error_report(self, message):
        if len(self.log_errors) or len(self.log_warnings):
            message += " with {0} Errors and {1} Warnings".format(
                len(self.log_errors), len(self.log_warnings)
            )
            if not bpy.app.background:
                bpy.context.window_manager.popup_menu(
                    self.list_errors, title=get_id("exporter_report_menu")
                )

            print(
                "{} Errors and {} Warnings".format(
                    len(self.log_errors), len(self.log_warnings)
                )
            )
            for msg in self.log_errors:
                print("Error:", msg)
            for msg in self.log_warnings:
                print("Warning:", msg)

        self.report({"INFO"}, message)
        print(message)
