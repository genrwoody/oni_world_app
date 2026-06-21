#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#include <ctime>
#include <iostream>
#include <fstream>
#define EMSCRIPTEN_KEEPALIVE
#endif

#include "App.hpp"

extern "C" {
void EMSCRIPTEN_KEEPALIVE app_init(int seed);
// the max mix is 5^17, bigger than 2^31 and smaller than 2^53, use double.
bool EMSCRIPTEN_KEEPALIVE app_generate(int type, int seed, double mix);
}

static App app;

void app_init(int seed) { app.Initialize(seed); }

bool app_generate(int type, int seed, double mix1)
{
    int traits = 0;
    if (seed < 0) {
        traits = -seed;
        seed = 0;
    }
    uint64_t mix = static_cast<uint64_t>(mix1);
    return app.Generate(type, seed, mix, traits);
}

#ifndef __EMSCRIPTEN__

int main()
{
    int type, seed;
    double mixing;
    app_init(time(nullptr));
    while (true) {
        std::cout << "input type, seed, mixing: ";
        std::cin >> type >> seed >> mixing;
        if (std::cin.eof()) {
            break;
        }
        if (!app_generate(type, seed, mixing)) {
            std::cout << "generate failed." << std::endl;
        }
    }
    return 0;
}

void jsExchangeData(uint32_t type, uint32_t count, size_t data)
{
    const char *geysers[] = {
        "低温蒸汽喷孔", "蒸汽喷孔",     "清水泉",         "低温泥浆泉",
        "污水泉",       "低温盐泥泉",   "盐水泉",         "小型火山",
        "火山",         "二氧化碳泉",   "二氧化碳喷孔",   "氢气喷孔",
        "高温污氧喷孔", "含菌污氧喷孔", "氯气喷孔",       "天然气喷孔",
        "铜火山",       "铁火山",       "金火山",         "铝火山",
        "钴火山",       "渗油裂缝",     "液硫泉",         "低温氯气喷孔",
        "钨火山",       "铌火山",       "污染浓盐水喷孔", "储油石",
        "潮汐泉",       "热气裂缝",     "输出端",         "输入端",
        "传送器",       "低温箱",       "打印舱"};
    const char *traits[] = {
        "坠毁的卫星群", "冰封之友",   "不规则的原油区",   "繁茂核心",
        "金属洞穴",     "放射性地壳", "地下海洋",         "火山活跃",
        "大型石块",     "中型石块",   "混合型石块",       "小型石块",
        "被圈闭的原油", "冰冻核心",   "活跃性地质",       "晶洞",
        "休眠性地质",   "大型冰川",   "不规则的原油区",   "岩浆通道",
        "金属贫瘠",     "金属富足",   "备选的打印舱位置", "粘液菌团",
        "地下海洋",     "火山活跃"};
    switch (type) {
    default:
        break;
    case RT_Starting:
        break;
    case RT_Trait: {
        auto ptr = (uint32_t *)data;
        auto end = ptr + count;
        while (ptr < end) {
            auto index = *ptr++;
            std::cout << traits[index] << std::endl;
        }
        break;
    }
    case RT_Geyser: {
        auto ptr = (uint32_t *)data;
        auto end = ptr + count;
        while (ptr < end) {
            auto index = *ptr++;
            auto x = *ptr++;
            auto y = *ptr++;
            std::cout << geysers[index] << ": " << x << ", " << y << std::endl;
        }
        break;
    }
    case RT_Polygon:
    case RT_WorldSize:
        break;
    case RT_Resource: {
        auto ptr = (char *)data;
        *ptr = 'E'; // if open file failed, it leads miniz to failed.
        std::ifstream fstm("data.bin", std::ios::binary);
        if (fstm.is_open()) {
            auto size = fstm.seekg(0, std::ios::end).tellg();
            if (size == count + 40) {
                fstm.seekg(40, std::ios::beg).read(ptr, count);
            } else {
                std::cout << "wrong count." << std::endl;
            }
        } else {
            std::cout << "can not open file." << std::endl;
        }
        break;
    }
    }
}

#endif
