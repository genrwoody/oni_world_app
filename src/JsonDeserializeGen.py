import sys
from datetime import date, datetime
from pathlib import Path

class JsonSerializeGenerater:
    header: str = ""
    output: str = ""
    # 0: none, 1: struct, 2: enum
    scope: int = 0
    ignore: bool = False
    name: str = ""

    def parse_line(self, line: str):
        line = line.strip()
        if line.isspace() or line.startswith("{"):
            return
        if line == "// ignore lines":
            self.ignore = True
        if line.startswith("/"):
            return
        words = line.split(" ")
        if words[0] == "};":
            self.ignore = False
            if self.scope == 1:
                self.output += """
        if ((int)value.size() != count) {{
            LogE("object {} parse failed.");
            return false;
        }}
        return true;
    }}
}};
""".format(self.name)
            elif self.scope == 2:
                self.output = self.output[:-2]
                self.output += """
        }};
        auto itr = dict.find(name);
        if (itr == dict.end()) {{
            LogE("enum {}::%s parse failed.", name.c_str());
            return false;
        }}
        obj = itr->second;
        return true;
    }}
}};
""".format(self.name)
            self.scope = 0
        elif self.ignore:
            return
        elif words[0] == "struct":
            self.scope = 1
            self.name = words[1]
            self.output += """
template<>
struct Deserializer<{}>
{{
    static bool deserialize(const Json::Value &value, {} &obj)
""".format(self.name, self.name)
            self.output += """    {
        const Json::Value *ptr = nullptr;
        int count = 0;
"""
        elif self.scope == 1 and len(words) > 1:
            member = ""
            if words[0].startswith("std::map"):
                member = words[2]
            else:
                member = words[1]
            if "(" in member:
                return
            if member.endswith("{};"):
                member = member[:-3]
            elif member.endswith(";") or member.endswith("{"):
                member = member[:-1]
            self.output += """        if ((ptr = value.find("{}")) != nullptr)
            count += Setting::deserialize(*ptr, obj.{}) ? 1 : 0;
""".format(member, member)
        elif words[0] == "enum":
            self.scope = 2
            self.name = words[2]
            self.output += """
template<>
struct Deserializer<{}>
{{
    static bool deserialize(const Json::Value &value, {} &obj)
    {{
        if (value.isInt()) {{
            obj = ({})value.asInt();
            return true;
        }}
        auto name = value.asString();
        static std::map<std::string, {}> dict = {{
""".format(self.name, self.name, self.name, self.name)
        elif self.scope == 2 and len(words) >= 1:
            member = words[0]
            if member.endswith(","):
                member = member[:-1]
            self.output += '            {{ "{}", {}::{} }},\n'.format(
                member, self.name, member)

    def generate(self, file: Path):
        skip = [
            "SettingsCache.hpp",
            "JsonDeserialize.hpp",
            "JsonDeserializeGen.hpp",
            "ComposableDictionary.hpp",
            "Vector2f.hpp"
        ]
        if file.name in skip:
            return
        print(f"generate file: {file}")
        self.header += '#include "Setting/{}"\n'.format(file.name)
        with open(file, "r") as stm:
            for line in stm:
                self.parse_line(line)

    def walk(self, path: str):
        directory = Path(path)
        for file in directory.rglob("*.hpp"):
            self.generate(file)
        with open("JsonDeserializeGen.hpp", "w") as stm:
            stm.write("""/* {} Generate by tool, Do *NOT* edit! */
#pragma once

#include "Setting/JsonDeserialize.hpp"
""".format(datetime.now().ctime()))
            stm.write(self.header)
            stm.write(self.output)

def main():
    jsg = JsonSerializeGenerater()
    jsg.walk(sys.argv[1])

if __name__ == "__main__":
    main()
