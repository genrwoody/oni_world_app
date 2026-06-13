import sys
import yaml
import json
import re
from pathlib import Path

excludes = [
    Path("/worldgen/noise/"),
    Path("/worldgen/storytraits/"),
    Path("/templates/storytraits/"),
    Path("/templates/dev_tests/"),
    Path("/templates/rough unused poi/"),
]

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

def is_clusters(source: str):
    return str(Path("/worldgen/clusters/")) in source

def handle_clusters(obj: dict, source: Path):
    if "skip" in obj and obj["skip"]:
        return False
    remove = ["poiPlacements", "name", "description", "welcomeMessage", \
              "clusterAudio", "clusterUnlocks", "dlcIdFrom", "difficulty", \
              "menuOrder", "mixedPoiPlacements"]
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
        (is_clusters, handle_clusters),
        (is_features, handle_features),
        (is_traits, handle_traits),
        (is_worlds, handle_worlds),
        (is_templates, handle_templates),
    ]
    for condition, function in dispatcher:
        if condition(str(source)):
            return function(obj, source)
    return True

def convert_yaml_to_json(source: Path, target: Path):
    for exclude in excludes:
        if str(exclude) in str(source):
            print(f"skip: {source}")
            return
    target.parent.mkdir(parents = True, exist_ok = True)
    with open(source, "r") as stm:
        obj = yaml.safe_load(stm)
        if not handle(obj, source):
            print(f"handle skip: {source}")
            return
        if target.name == "GeoDormant.json":
            target = target.parent / "Geodormant.json"
        with open(target, "w", newline="") as out:
            chunk = json.dumps(obj, indent = 2)
            out.write(chunk)
            if chunk.endswith("}"):
                out.write("\n")

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
            target = destpath / source.relative_to(basepath)
            target = target.with_suffix(".json")
            convert_yaml_to_json(source, target)

if __name__ == "__main__":
    main()
