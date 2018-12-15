import json
import os
import struct
import sys
import re
import math

PROGNAME = os.path.basename(sys.argv[0])

class TiledJSONDecoder(json.JSONDecoder):
    def default(self, obj):
        return obj.__dict__

class AABB(object):
    def __init__(self, (cx, cy), (minx, miny), (maxx, maxy)):
        self.cx = int(cx)
        self.cy = int(cy)
        self.minx = int((minx))
        self.miny = int((miny))
        self.maxx = int(round(maxx))
        self.maxy = int(round(maxy))
        self.width = int(maxx - minx)
        self.height = int(maxy - miny)
        self.hwidth = int(round(maxx - minx) / 2.0)
        self.hheight = int(round(maxy - miny) / 2.0)

    def __str__(self):
        return ("center: (%i, %i), min: (%i, %i), max: (%i, %i)"
                    % (self.cx, self.cy, self.minx, self.miny, self.maxx, self.maxy))

class TiledObjectGroup(object):
    def __init__(self, layer):
        self.aabbs = []
        self.types = []

    @property
    def count(self):
        return self._count

    def _generate_data(self, objects):
        self._count = len(objects)
        for object_ in objects:
            cx = object_["x"]
            cy = object_["y"]
            width = object_["width"]
            height = object_["height"]
            hwidth = width / 2.0
            hheight = height / 2.0
            minx = -hwidth;
            miny = -hheight;
            maxx = hwidth;
            maxy = hheight;
            # print (minx, miny, maxx, maxy, cx, cy)
            aabb = AABB((cx, cy), (minx, miny), (maxx, maxy))
            self.aabbs.append(aabb)
            type_ = object_["type"]
            self.types.append(type_)

class TiledTile(TiledObjectGroup):
    def __init__(self, tile_number, tile):
        super(TiledTile, self).__init__(tile)
        self.number = int(tile_number)
        objectgroup = tile["objectgroup"]
        objects = objectgroup["objects"]
        self._generate_data(objects)

class TiledTrigger(TiledObjectGroup):
    def __init__(self, layer):
        super(TiledTrigger, self).__init__(layer)
        self.name = layer["name"]
        objects = layer["objects"]
        self._generate_data(objects)

class TiledLayer(object):
    def __init__(self, layer):
        self.name = layer["name"]
        self.data = layer["data"]
        self.twidth = layer["width"]
        self.theight = layer["height"]
        # XXX: These are accessible already, no need to calculate
        self.width = self.twidth * 16
        self.height = self.theight * 16
        # XXX: Are these even useful?
        # XXX: What about page info per plane?
        self.scroll_screen = layer["properties"]["scroll_screen"]
        self.cpd_address = int(layer["properties"]["cpd_address"], 0)
        self.color_palette_address = int(layer["properties"]["color_palette_address"], 0)
        # XXX: Hard coded for now
        self.bpp = 8
        self.map = []
        self.pnd_map = []
        for entry in self.data:
            # 0 in Tiled is designated as empty, not tile #0
            clamped_entry = 0 if ((entry - 1) < 0) else (entry - 1)
            self.map.append(clamped_entry)
            self.pnd_map.append(self._pnd_config_6(clamped_entry, 0, 0))

    def _character_number(sel, addr):
        return (addr >> 5)

    def _palette_number(self, addr):
        return (addr >> 5)

    def _pnd_config_6(self, index, vf, hf):
        return ((((self._palette_number(self.color_palette_address) >> 4) & 0x0007) << 12) |
                (((vf) & 0x01) << 11) |
                (((hf) & 0x01) << 10) |
                ((self._character_number(self.cpd_address + (index * (1 << self.bpp))) & 0x0FFC) >> 2))

def output_tiled_tiles(tiled_tileset, tiled_tiles, tiled_layers, output_path):
    col_name = tiled_tileset["name"][0:8].upper()
    col_filename = "%s.COL" % (col_name)
    col_path = os.path.join(output_path, col_filename)

    # XXX: Hard coded map size

    with open(col_path, "w+") as ofp:
        ofp.write(struct.pack(">H", 0))
        count = 0
        # Dump collision info
        for tiled_layer in tiled_layers:
            for page_y in range(32):
                for page_x in range(32):
                    index = tiled_layer.map[page_x + (page_y * 32)] - 1
                    if index < 0:
                        continue
                    tiled_tile = tiled_tiles[index]
                    if tiled_tile is None:
                        continue
                    for type_, aabb in zip(tiled_tile.types, tiled_tile.aabbs):
                        # XXX: For now map directly TC_WALL=1
                        # XXX: Fix later
                        cx = aabb.cx + (page_x * 16) + aabb.hwidth
                        cy = aabb.cy + (page_y * 16) + aabb.hheight
                        ofp.write(struct.pack(">H", 1))
                        ofp.write(struct.pack(">hhhhhh", cx, cy, aabb.minx, aabb.miny, aabb.maxx, aabb.maxy))
                        # Callback #1
                        ofp.write(struct.pack(">L", 0))
                        print "Tile AABB: %s" % (aabb)
                        count += 1
        ofp.seek(0, 0)
        ofp.write(struct.pack(">H", count))

def output_tiled_triggers(tiled_trigger, output_path):
    obj_name = tiled_trigger.name[0:8].upper()
    obj_filename = "%s.TRG" % (obj_name)
    obj_path = os.path.join(output_path, obj_filename)

    with open(obj_path, "w+") as ofp:
        ofp.write(struct.pack(">H", tiled_trigger.count))
        for type_, aabb in zip(tiled_trigger.types, tiled_trigger.aabbs):
            cx = aabb.cx + aabb.hwidth
            cy = aabb.cy + aabb.hheight
            ofp.write(struct.pack(">H", 1))
            ofp.write(struct.pack(">hhhhhh", cx, cy, aabb.minx, aabb.miny, aabb.maxx, aabb.maxy))
            # Callback #1
            ofp.write(struct.pack(">L", 0))
            print "Trigger AABB: %s" % (aabb)

def output_tiled_layer(tiled_layer, output_path):
    """
    """
    map_name = tiled_layer.name[0:8].upper()
    map_filename = "%s.MAP" % (map_name)
    map_path = os.path.join(output_path, map_filename)
    buf = struct.pack(">%sH" % (len(tiled_layer.pnd_map)), *tiled_layer.pnd_map)
    with open(map_path, "w+") as ofp:
        ofp.write(buf)

def parse_tiled_layers(tiled):
    """
    Parse only what is necessary from tiled layers.
    """
    tiled_layers = []
    for layer in tiled["layers"]:
        if layer["type"] == "tilelayer":
            tiled_layer = TiledLayer(layer)
            tiled_layers.append(tiled_layer)
    return tiled_layers

def parse_tiled_tiles(tiled_tileset):
    """
    Parse only what is necessary from tiled tileset.
    """
    tile_count = tiled_tileset["tilecount"]
    tiled_tiles = [None] * tile_count
    for tile_number, tile in tiled_tileset["tiles"].items():
        tn = int(tile_number)
        clamped_tn = 0 if ((tn - 1) < 0) else (tn - 1)
        if clamped_tn == 0:
            continue
        tiled_tile = TiledTile(clamped_tn, tile)
        if tiled_tile.count > 0:
            tiled_tiles[clamped_tn] = tiled_tile
    return tiled_tiles

def parse_tiled_triggers(tiled):
    """
    """
    tiled_triggers = []
    for layer in tiled["layers"]:
        if layer["type"] == "objectgroup":
            tiled_trigger = TiledTrigger(layer)
            tiled_triggers.append(tiled_trigger)
    return tiled_triggers

def main():
    def usage():
        print >> sys.stderr, "%s in.json output-path" % (PROGNAME)

    if len(sys.argv[1:]) != 2:
        usage()
        sys.exit(2)

    json_filename = sys.argv[1]
    json_path = os.path.dirname(json_filename)
    output_path = sys.argv[2]

    # Read and parse JSON file containing layers
    try:
        with open(json_filename, "r") as ifp:
            tiled = json.load(ifp, cls = TiledJSONDecoder)
    except IOError as e:
        print >> sys.stderr, "%s: error: %s" % (PROGNAME, e.strerror)

    # Read and parse JSON tileset file
    try:
        tileset_filename = os.path.join(json_path, tiled["tilesets"][0]["source"])
        with open(tileset_filename, "r") as ifp:
            tiled_tileset = json.load(ifp, cls = TiledJSONDecoder)
    except IOError as e:
        print >> sys.stderr, "%s: error: %s, %s" % (PROGNAME, tileset_filename, e.strerror)

    tiled_layers = parse_tiled_layers(tiled)
    for tiled_layer in tiled_layers:
        output_tiled_layer(tiled_layer, output_path)

    tiled_tiles = parse_tiled_tiles(tiled_tileset)
    output_tiled_tiles(tiled_tileset, tiled_tiles, tiled_layers, output_path)

    tiled_triggers = parse_tiled_triggers(tiled)
    for tiled_trigger in tiled_triggers:
        output_tiled_triggers(tiled_trigger, output_path)

if __name__ == '__main__':
    main()
