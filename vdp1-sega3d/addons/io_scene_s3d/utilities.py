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
from mathutils import Matrix, Vector, Euler, Quaternion
from math import *


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
                    self.list_errors, title="Source Tools Error Report"
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
