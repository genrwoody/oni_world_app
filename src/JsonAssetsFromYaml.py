import sys
import yaml
import json
import re
import heapq
import hashlib
from pathlib import Path

excludes = [
    Path("/worldgen/layers"),
    Path("/worldgen/rooms"),
    Path("/worldgen/rivers"),
    Path("/worldgen/borders"),
    Path("/worldgen/mobs"),
    Path("/worldgen/mixing"),
    Path("/worldgen/biomes"),
    Path("/worldgen/noise/"),
    Path("/worldgen/storytraits/"),
    Path("/templates/storytraits/"),
    Path("/templates/dev_tests/"),
    Path("/templates/rough unused poi/"),
]

heap = [] # show the top ten files by size.
sha1 = hashlib.sha1() # calculate the sha1 of all files(with path).
float_regex = re.compile(r"^[+-]?\.\d+$")

# fix float number like: -.1, +.2
def fix_float_number(num: str):
    if float_regex.match(num):
        return float(num.replace(".", "0."))
    raise ValueError(f"Can not convert str '{num}' to float")

def refresh_info(cells: list, info: dict):
    min_x = 1
    max_x = -1
    min_y = 1
    max_y = -1
    for cell in cells:
        if "location_x" not in cell:
            cell["location_x"] = 0
        if cell["location_x"] < min_x:
            min_x = cell["location_x"]
        if cell["location_x"] > max_x:
            max_x = cell["location_x"]
        if "location_y" not in cell:
            cell["location_y"] = 0
        if cell["location_y"] < min_y:
            min_y = cell["location_y"]
        if cell["location_y"] > max_y:
            max_y = cell["location_y"]
    info["size"] = {"X": (max_x - min_x) + 1, "Y": (max_y - min_y) + 1}
    info["min"] = {"X": min_x, "Y": min_y}
    info["area"] = len(cells)
    if "discover_tags" in info:
        del info["discover_tags"]

def is_temperatures(source: str):
    return str(Path("/worldgen/temperatures")) in source

def handle_temperatures(obj: dict, source: Path):
    if "remove" in obj and obj["remove"]:
        print("error: unsupported temperatures.yaml")
        return False
    add = obj["add"]
    obj.clear()
    obj.update(add)
    return True

def is_clusters(source: str):
    return str(Path("/worldgen/clusters/")) in source

def handle_clusters(obj: dict, source: Path):
    if "skip" in obj and obj["skip"]:
        return False
    remove = ["poiPlacements", "name", "description", "welcomeMessage", \
              "clusterAudio", "clusterUnlocks", "dlcIdFrom", "difficulty", \
              "menuOrder", "mixedPoiPlacements", "startingMinions", "skip"]
    for key in remove:
        if key in obj:
            del obj[key]
    return True

def is_features(source: str):
    return str(Path("/worldgen/features/")) in source

def handle_features(obj: dict, source: Path):
    if "borders" in obj and obj["borders"] is None: # remove null value
        del obj["borders"]
    return True

def is_traits(source: str):
    return str(Path("/worldgen/traits/")) in source

def handle_traits(obj: dict, source: Path):
    if source.name == "MisalignedStart.yaml": # "-.1" -> -0.1
        for key, value in obj.items():
            if key == "startingBasePositionHorizontalMod" or \
                key == "startingBasePositionVerticalMod":
                for minmax, num in value.items():
                    if isinstance(num, str):
                        value[minmax] = fix_float_number(num)
    if source.name == "GeoActive.yaml": # GeoDormant -> Geodormant
        traits = obj["exclusiveWith"]
        obj["exclusiveWith"] = [
            trait.replace("GeoDormant", "Geodormant") for trait in traits
        ]
    remove = ["description", "colorHex", "icon"]
    for key in remove:
        if key in obj:
            del obj[key]
    return True

def is_worlds(source: str):
    return str(Path("/worldgen/worlds/")) in source

def handle_worlds(obj: dict, source: Path):
    if "skip" in obj and obj["skip"]:
        return False
    if "worldTraitRules" in obj: # GeoDormant -> Geodormant
        for rule in obj["worldTraitRules"]:
            if "forbiddenTraits" not in rule:
                continue
            traits = rule["forbiddenTraits"]
            rule["forbiddenTraits"] = [
                trait.replace("GeoDormant", "Geodormant") for trait in traits
            ]
    remove = ["description", "nameTables", "overrideName", "asteroidIcon", \
              "iconScale", "skip", "seasons"]
    for key in remove:
        if key in obj:
            del obj[key]
    return True

def is_subworlds(source: str):
    return str(Path("/worldgen/subworlds/")) in source

def handle_subworlds(obj: dict, source: Path):
    remove = ["nameKey", "descriptionKey", "utilityKey", "overrideNoise", \
              "biomeNoise", "densityNoise", "backwallNoise", "borderOverride"]
    for key in remove:
        if key in obj:
            del obj[key]
    return True

def is_templates(source: str):
    return "templates" in source and "worldgen" not in source

def handle_templates(obj: dict, source: Path):
    refresh_info(obj["cells"], obj["info"])
    del obj["cells"]
    if "buildings" in obj:
        del obj["buildings"]
    if "pickupables" in obj:
        del obj["pickupables"]
    if "elementalOres" in obj:
        del obj["elementalOres"]
    others=[]
    if "otherEntities" in obj:
        keys = ["id", "location_x", "location_y"]
        for entity in obj["otherEntities"]:
            copy = {}
            for k, v in entity.items():
                if k in keys:
                    copy[k] = v
            others.append(copy)
        obj["otherEntities"] = others
    return True

def handle(obj: dict, source: Path):
    dispatcher = [
        (is_temperatures, handle_temperatures),
        (is_clusters, handle_clusters),
        (is_features, handle_features),
        (is_traits, handle_traits),
        (is_worlds, handle_worlds),
        (is_subworlds, handle_subworlds),
        (is_templates, handle_templates),
    ]
    for condition, function in dispatcher:
        if condition(str(source)):
            return function(obj, source)
    return True

def convert_yaml_to_json(source: Path, target: Path, destpath: Path):
    for exclude in excludes:
        if str(exclude) in str(source):
            print(f"skip: {source}")
            return
    with open(source, "r") as stm:
        obj = yaml.safe_load(stm)
        if not handle(obj, source):
            print(f"handle skip: {source}")
            return
        if target.name == "GeoDormant.json":
            target = target.parent / "Geodormant.json"
        sha1.update(bytes(target))
        fullpath = destpath / target
        fullpath.parent.mkdir(parents = True, exist_ok = True)
        with open(fullpath, "w", newline="") as out:
            chunk = json.dumps(obj, indent = 2)
            out.write(chunk)
            sha1.update(chunk.encode("utf-8"))
            if len(heap) < 10:
                heapq.heappush(heap, (len(chunk), str(target)))
            elif len(chunk) > heap[0][0]:
                heapq.heappop(heap)
                heapq.heappush(heap, (len(chunk), str(target)))
            if chunk.endswith("}"):
                out.write("\n")
                sha1.update(b"\n")

def main():
    if len(sys.argv) < 2:
        print("Generate json format assets using yaml format assets.")
        print(f"usage: {sys.argv[0]} <THE_GAME_INSTALL_DIR>")
        print(f"example: {sys.argv[0]} C:/Games/OxygenNotIncluded")
        return
    gamepath = sys.argv[1]
    basepath = Path(gamepath + "/OxygenNotIncluded_Data/StreamingAssets/")
    destpath = Path(sys.argv[0] + "/../../asset").resolve()
    for subdir in ["dlc", "templates", "worldgen"]:
        for source in (basepath / subdir).rglob("*.yaml"):
            target = source.relative_to(basepath)
            target = target.with_suffix(".json")
            convert_yaml_to_json(source, target, destpath)
    with open(destpath / "sha1.txt", "w", newline="") as out:
        out.write(sha1.hexdigest())
    print("\nthe top ten files by size:")
    top10 = sorted(heap, reverse = True)
    for size, file in top10:
        print(f"file: {file}, size: {size}")

if __name__ == "__main__":
    main()
