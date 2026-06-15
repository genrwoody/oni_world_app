#pragma once

#include <cstdint>
#include <string>
#include <memory>

enum ResultType {
    RT_Starting,
    RT_Trait,
    RT_Geyser,
    RT_Polygon,
    RT_WorldSize,
    RT_Resource
};

// I defined only one function for exchanging data between c++ and js,
// it get resource from js and set result to js.
extern "C" void jsExchangeData(uint32_t type, uint32_t count, size_t data);

class SettingsCache;
class KRandom;

class App
{
private:
    std::unique_ptr<SettingsCache> m_settings;
    std::unique_ptr<KRandom> m_random;

public:
    App();
    ~App();

    void Initialize(int seed);
    bool Generate(const std::string &code, int traits);
    bool Generate(int type, int seed, uint64_t mix, int traits);

private:
    bool GenerateInternal(int traitsFlag);

private:
    App(const App &app) = delete;
    App &operator=(const App &app) = delete;

    App(App &&app) = delete;
    App &operator=(App &&app) = delete;
};
