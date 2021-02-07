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

from bpy.app.translations import pgettext
from mathutils import Matrix, Vector, Euler, Quaternion
from math import *

# fmt: off

EXPORTABLE_TYPES = (
    'MESH'
)
# fmt: on


global exportables
exportables = globals().get("exportables", set())


def get_id(id, format_string=False, data=False):
    from . import translations

    ids = translations.ids

    return ids[id]


def count_exports(context):
    count = 0
    for exportable in context.scene.s3d.export_list:
        id = exportable.get_id()
        if id and id.vs.export and (type(id) != bpy.types.Collection or not id.vs.mute):
            count += 1
    return count


def make_object_icon(object, prefix=None, suffix=None):
    if not (prefix or suffix):
        raise TypeError("A prefix or suffix is required")

    if object.type == "TEXT":
        type = "FONT"
    else:
        type = object.type

    out = ""
    if prefix:
        out += prefix
    out += type
    if suffix:
        out += suffix
    return out


def get_file_extension():
    return ".s3d"


def make_export_list():
    s = bpy.context.scene
    s.s3d.export_list.clear()

    def make_display_name(item, name=None):
        return os.path.join(
            item.s3d.subdir if item.s3d.subdir != "." else "",
            (name if name else item.name) + get_file_extension(),
        )

    if len(exportables):
        ungrouped_objects = exportables.copy()

        groups_sorted = bpy.data.collections[:]
        groups_sorted.sort(key=lambda g: g.name.lower())

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


from bpy.app.handlers import depsgraph_update_post, persistent

last_export_refresh = 0


@persistent
def scene_update(scene, immediate=False):
    global last_export_refresh
    global exportables

    if not hasattr(scene, "s3d"):
        return

    exportables = set([ob for ob in scene.objects if ob.type in EXPORTABLE_TYPES])

    now = time.time()

    if immediate or now - last_export_refresh > 0.25:
        make_export_list()
        last_export_refresh = now


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
        self.startTime = time.time()

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
        return round(time.time() - self.startTime, 1)

    def errorReport(self, message):
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
