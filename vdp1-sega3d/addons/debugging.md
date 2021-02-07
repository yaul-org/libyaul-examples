# Blender Development in VS Code

## Installation

VS Code (unknown version) is needed, as well as the /Blender Development in VS
Code/ extension. Open the extensions view by clicking /View/→/Extensions/.

## Working with an existing addon

Have your addon located outside of `%USERPROFILE%\AppData\Roaming\Blender
Foundation\Blender\2.82\scripts\addons`, and in VS Code, click /File/→/Open
Folder/, and select your addon directory containing `__init__.py`. The extension
only supports addons that have a proper folder structure. That is if your addon
is a single `.py` file, you have to convert it first. To do this, move the file
into a new empty folder and rename it to `__init__.py`. For example, the
`io_scene_xxx` directory should contain a `__init__.py` file inside.

Once you've opened the folder, Press /Ctrl+Shift+P/ and type in `Blender:
Start`. After you choose a path, Blender will open. The terminal output can be
seen inside of VS Code. The first time you open a new Blender build like this
can take a few seconds longer than usual because some Python libraries are
installed automatically. For that it is important that you have an internet
connection.

Once Blender is started, you can use the addon in Blender. Debugging should work
now.
