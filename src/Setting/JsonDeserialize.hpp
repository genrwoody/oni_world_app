#pragma once

#include <cstdio>
#include <vector>
#include <string>
#include <variant>
#include <map>
#include <optional>

#include <json/value.h>

#include "Utils/Vector2f.hpp"
#include "Utils/Log.hpp"

template<typename T>
struct Deserializer {
    static bool deserialize(const Json::Value & /*json*/, T & /*obj*/)
    {
        static_assert(false, "unsupport");
    }
};

namespace Setting
{
template<typename T>
bool deserialize(const Json::Value &json, T &obj)
{
    if (json.isNull())
        return true;
    return Deserializer<T>::deserialize(json, obj);
}
} // namespace Setting

template<>
struct Deserializer<bool> {
    static bool deserialize(const Json::Value &json, bool &obj)
    {
        obj = json.asBool();
        return true;
    }
};

template<>
struct Deserializer<int> {
    static bool deserialize(const Json::Value &json, int &obj)
    {
        obj = json.asInt();
        return true;
    }
};

template<>
struct Deserializer<float> {
    static bool deserialize(const Json::Value &json, float &obj)
    {
        obj = json.asFloat();
        return true;
    }
};

template<>
struct Deserializer<std::string> {
    static bool deserialize(const Json::Value &json, std::string &obj)
    {
        obj = json.asString();
        return true;
    }
};

template<typename T>
struct Deserializer<std::vector<T>> {
    static bool deserialize(const Json::Value &value, std::vector<T> &obj)
    {
        if (!value.isArray()) {
            return false;
        }
        int count = 0;
        obj.reserve(value.size());
        for (auto &item : value) {
            T &tmp = obj.emplace_back();
            count += Deserializer<T>::deserialize(item, tmp) ? 1 : 0;
        }
        if ((int)value.size() != count) {
            LogE("object std::vector parse failed.");
            return false;
        }
        return true;
    }
};

template<typename T>
struct Deserializer<std::map<std::string, T>> {
    static bool deserialize(const Json::Value &value,
                            std::map<std::string, T> &obj)
    {
        int count = 0;
        for (auto itr = value.begin(); itr != value.end(); ++itr) {
            auto key = itr.name();
            count += Deserializer<T>::deserialize(*itr, obj[key]) ? 1 : 0;
        }
        if ((int)value.size() != count) {
            LogE("object std::map parse failed.");
            return false;
        }
        return true;
    }
};

template<typename T>
struct Deserializer<std::optional<T>> {
    static bool deserialize(const Json::Value &value, std::optional<T> &obj)
    {
        return Deserializer<T>::deserialize(value, obj.emplace());
    }
};

template<>
struct Deserializer<std::variant<bool, float, std::string>> {
    using Variant = std::variant<bool, float, std::string>;
    static bool deserialize(const Json::Value &json, Variant &obj)
    {
        if (json.isBool()) {
            obj = json.asBool();
        } else if (json.isString()) {
            obj = json.asString();
        } else {
            obj = json.asFloat();
        }
        return true;
    }
};

template<>
struct Deserializer<Vector2f> {
    static bool deserialize(const Json::Value &value, Vector2f &obj)
    {
        int count = 0;
        const Json::Value *ptr = nullptr;
        if ((ptr = value.find("X")) != nullptr)
            count += Setting::deserialize(*ptr, obj.x) ? 1 : 0;
        if ((ptr = value.find("Y")) != nullptr)
            count += Setting::deserialize(*ptr, obj.y) ? 1 : 0;

        if ((int)value.size() != count) {
            LogE("object Vector2f parse failed.");
            return false;
        }
        return true;
    }
};
