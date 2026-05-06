#include "WorldGen.hpp"

#include <iostream>
#include <algorithm>
#include <functional>
#include <string_view>

#include "Utils/PointGenerator.hpp"
#include "Utils/SortHelper.hpp"

using TagSet = std::vector<std::string>;

class TemplateSpawning
{
private:
    int m_poiPadding = 0;
    const World &m_world;
    const SettingsCache &m_settings;
    KRandom &m_random;
    std::vector<Site *> m_sites;
    std::vector<Rect> m_poiBounds;
    std::vector<TemplateSpawner> m_templates;

public:
    TemplateSpawning(KRandom &random, const World &world,
                     const SettingsCache &settings)
        : m_world{world}
        , m_settings{settings}
        , m_random{random}
    {
    }

    void DetermineTemplatesForWorld(std::vector<Site *> &sites)
    {
        m_sites.swap(sites);
        m_poiPadding = (int)GetDefaultData<float>("POIPadding");

        DrawWorldBorder();
        SpawnStartingTemplate();
        SpawnTemplatesFromTemplateRules();

        sites.swap(m_sites);
    }

    std::vector<TemplateSpawner> &GetTemplates() { return m_templates; }

private:
    template<typename T>
    const T &GetDefaultData(const std::string &key) const
    {
        return m_settings.GetDefaultData<T>(m_world, key);
    }

    void DrawWorldBorder();
    void SpawnStartingTemplate();
    void SpawnTemplatesFromTemplateRules();
    bool IsPOIOverlappingBounds(const Rect &templateBounds) const;
    bool RemoveOverlappingPOI(const Site *site, const TemplateContainer &templt,
                              const TemplateSpawnRules &rule) const;
    bool ApplyTemplateRules(const TemplateSpawnRules &rule,
                            std::set<std::string> &usedTemplates);
    Site *FindTargetForTemplate(const TemplateContainer &templt,
                                const TemplateSpawnRules &rule, bool guarantee);
};

bool TemplateSpawning::IsPOIOverlappingBounds(const Rect &templateBounds) const
{
    for (auto &placedPOIBound : m_poiBounds) {
        if (templateBounds.Overlaps(placedPOIBound)) {
            return true;
        }
    }
    return false;
}

void TemplateSpawning::SpawnStartingTemplate()
{
    auto itr = std::ranges::find_if(m_sites, [](Site *site) {
        return site->tags.contains("StartLocation");
    });
    if (itr == m_sites.end()) {
        LogE("can not find start location.");
        return;
    }
    auto pair = m_settings.templates.find(m_world.startingBaseTemplate);
    if (pair == m_settings.templates.end()) {
        LogE("can not find start template.");
        return;
    }
    auto &container = pair->second;
    Vector2f position{(*itr)->x, (*itr)->y};
    Rect templateBounds = container.info.GetBounds(position, m_poiPadding);
    if (IsPOIOverlappingBounds(templateBounds)) {
        LogE("start location should not overlap bounds.");
        return;
    }
    m_poiBounds.push_back(templateBounds);
}

static void UpdateNodeTags(Site &site, const std::string &name, bool remove)
{
    if (remove) {
        site.templateTag = "Invalid";
        site.tags.erase(name);
        site.tags.erase("POI");
    } else {
        site.templateTag = name;
        site.tags.emplace(name);
        site.tags.emplace("POI");
    }
}

static bool IsSafeToSpawnPOI(const Site *site, const TagSet &tags)
{
    for (auto &tag : tags) {
        if (site->tags.contains(tag)) {
            return false;
        }
    }
    return true;
}

static bool IsSafeToSpawnPOI(const Site *site)
{
    TagSet tags{"StartLocation", "AtStart", "NearStartLocation", "POI",
                "Feature"};
    return IsSafeToSpawnPOI(site, tags);
}

static bool IsSafeToSpawnPOIRelaxed(const Site *site)
{
    TagSet tags{"StartLocation", "AtStart", "NearStartLocation", "POI"};
    return IsSafeToSpawnPOI(site, tags);
}

static bool IsSafeToSpawnPOINearStart(const Site *site)
{
    TagSet tags{"POI", "Feature"};
    return IsSafeToSpawnPOI(site, tags);
}

static bool IsSafeToSpawnPOINearStartRelaxed(const Site *site)
{
    TagSet tags{"POI"};
    return IsSafeToSpawnPOI(site, tags);
}

static bool DoesCellMatchFilter(const Site &site,
                                const AllowedCellsFilter &filter, bool &applied)
{
    applied = true;
    switch (filter.tagcommand) {
    case TagCommand::AtTag:
        return site.tags.contains(filter.tag);
    case TagCommand::NotAtTag:
        return !site.tags.contains(filter.tag);
    case TagCommand::Default:
        if (!filter.subworldNames.empty()) {
            for (auto &subworldName : filter.subworldNames) {
                if (site.tags.contains(subworldName)) {
                    return true;
                }
            }
            return false;
        }
        if (!filter.zoneTypes.empty()) {
            for (auto &zoneType : filter.zoneTypes) {
                if (site.tags.contains(ZoneTypeToString(zoneType))) {
                    return true;
                }
            }
            return false;
        }
        if (!filter.temperatureRanges.empty()) {
            for (auto &range : filter.temperatureRanges) {
                if (site.tags.contains(TempRangeToString(range))) {
                    return true;
                }
            }
            return false;
        }
        return true;
    case TagCommand::DistanceFromTag: {
        auto itr = site.parent->minDistanceToTag.find(filter.tag);
        if (itr != site.parent->minDistanceToTag.end()) {
            if (itr->second >= filter.minDistance) {
                return itr->second <= filter.maxDistance;
            }
            return false;
        }
        applied = false;
        return true;
    }
    }
    return false;
}

static bool DoesCellMatchFilters(const Site &site,
                                 const std::vector<AllowedCellsFilter> &filters)
{
    bool flag = false;
    for (auto &filter : filters) {
        bool applied;
        bool flag2 = DoesCellMatchFilter(site, filter, applied);
        if (!applied) {
            continue;
        }
        switch (filter.command) {
        case Command::All:
            flag = true;
            break;
        case Command::Clear:
            flag = false;
            break;
        case Command::Replace:
            flag = flag2;
            break;
        case Command::ExceptWith:
        case Command::SymmetricExceptWith:
            if (flag2) {
                flag = false;
            }
            break;
        case Command::UnionWith:
            flag = flag2 || flag;
            break;
        case Command::IntersectWith:
            flag = flag2 && flag;
            break;
        }
    }
    return flag;
}

bool TemplateSpawning::RemoveOverlappingPOI(
    const Site *site, const TemplateContainer &templt,
    const TemplateSpawnRules &rule) const
{
    auto position = site->polygon.Centroid() + rule.overrideOffset;
    Rect templateBounds = templt.info.GetBounds(position, m_poiPadding);
    if (IsPOIOverlappingBounds(templateBounds)) {
        return true;
    } else if (rule.allowExtremeTemperatureOverlap) {
        return false;
    }
    templateBounds.width += 6.0f - 2.0f * m_poiPadding;
    templateBounds.height += 6.0f - 2.0f * m_poiPadding;
    auto temperatureRange1 = site->subworld->temperatureRange;
    auto itr = m_settings.temperatures.find(temperatureRange1);
    if (itr == m_settings.temperatures.end()) {
        LogE("can not find range: %d.", (int)temperatureRange1);
        return true;
    }
    auto &temperature1 = itr->second;
    for (auto item : m_sites) {
        auto temperatureRange2 = item->subworld->temperatureRange;
        if (temperatureRange1 == temperatureRange2) {
            continue;
        }
        auto itr2 = m_settings.temperatures.find(temperatureRange2);
        if (itr2 == m_settings.temperatures.end()) {
            LogE("can not find range: %d.", (int)temperatureRange2);
            return true;
        }
        auto &temperature2 = itr2->second;
        float min = std::min(temperature1.min, temperature2.min);
        float max = std::max(temperature1.max, temperature2.max);
        bool flag = templateBounds.Overlaps(item->polygon.Bounds());
        if (flag && max - min > 100.0f) {
            return true;
        }
    }
    return false;
}

Site *TemplateSpawning::FindTargetForTemplate(const TemplateContainer &templt,
                                              const TemplateSpawnRules &rule,
                                              bool guarantee)
{
    std::vector<Site *> filtered;
    for (auto site : m_sites) {
        if (rule.allowNearStart) {
            if (IsSafeToSpawnPOINearStart(site) &&
                DoesCellMatchFilters(*site, rule.allowedCellsFilter))
                filtered.push_back(site);
        } else if (rule.useRelaxedFiltering) {
            if (IsSafeToSpawnPOIRelaxed(site) &&
                DoesCellMatchFilters(*site, rule.allowedCellsFilter))
                filtered.push_back(site);
        } else {
            if (IsSafeToSpawnPOI(site) &&
                DoesCellMatchFilters(*site, rule.allowedCellsFilter))
                filtered.push_back(site);
        }
    }
    auto itr = std::ranges::remove_if(
        filtered, [this, &templt, &rule](const Site *site) {
            return RemoveOverlappingPOI(site, templt, rule);
        });
    filtered.erase(itr.begin(), itr.end());
    if (filtered.empty() && guarantee) {
        for (auto site : m_sites) {
            if (rule.allowNearStart && rule.useRelaxedFiltering) {
                if (IsSafeToSpawnPOINearStartRelaxed(site) &&
                    DoesCellMatchFilters(*site, rule.allowedCellsFilter))
                    filtered.push_back(site);
            } else if (!rule.useRelaxedFiltering) {
                if (IsSafeToSpawnPOIRelaxed(site) &&
                    DoesCellMatchFilters(*site, rule.allowedCellsFilter))
                    filtered.push_back(site);
            }
        }
        auto itr2 = std::ranges::remove_if(
            filtered, [this, &templt, &rule](const Site *site) {
                return RemoveOverlappingPOI(site, templt, rule);
            });
        filtered.erase(itr2.begin(), itr2.end());
    }
    if (filtered.empty()) {
        return nullptr;
    }
    ShuffleSeeded(filtered, m_random);
    return filtered.back();
}

bool TemplateSpawning::ApplyTemplateRules(const TemplateSpawnRules &rule,
                                          std::set<std::string> &usedTemplates)
{
    std::vector<std::string_view> rules;
    for (auto &name : rule.names) {
        if (!rule.allowDuplicates && usedTemplates.contains(name)) {
            continue;
        }
        if (m_settings.templates.contains(name)) {
            rules.push_back(name);
        }
    }
    if (rules.empty()) {
        return false;
    }
    ShuffleSeeded(rules, m_random);
    int num = 0;
    if (rule.listRule == ListRule::GuaranteeRange ||
        rule.listRule == ListRule::TryRange) {
        num = m_random.Next(rule.range.x, rule.range.y);
    }
    int minCount = 0;
    int maxCount = 0;
    switch (rule.listRule) {
    case ListRule::GuaranteeAll:
        minCount = (int)rules.size();
        maxCount = (int)rules.size();
        break;
    case ListRule::GuaranteeSome:
        minCount = rule.someCount;
        maxCount = rule.someCount;
        break;
    case ListRule::GuaranteeSomeTryMore:
        minCount = rule.someCount;
        maxCount = rule.someCount + rule.moreCount;
        break;
    case ListRule::GuaranteeOne:
        minCount = 1;
        maxCount = 1;
        break;
    case ListRule::TryAll:
        maxCount = (int)rules.size();
        break;
    case ListRule::TrySome:
        maxCount = rule.someCount;
        break;
    case ListRule::TryOne:
        maxCount = 1;
        break;
    case ListRule::GuaranteeRange:
        minCount = num;
        maxCount = num;
        break;
    case ListRule::TryRange:
        maxCount = num;
        break;
    }
    for (auto &item : rules) {
        if (maxCount <= 0) {
            break;
        }
        std::string name(item);
        auto itr = m_settings.templates.find(name);
        if (itr == m_settings.templates.end()) {
            LogE("can not find template: %s", name.c_str());
            return false;
        }
        auto &templt = itr->second;
        bool guarantee = minCount > 0;
        Vector2f position;
        Site *site = nullptr;
        if (rule.overridePlacement.x != -1.0f) {
            position = rule.overridePlacement;
            auto itr2 = std::find_if(m_sites.begin(), m_sites.end(),
                                    [&position](Site *site) {
                                        return site->polygon.Contains(position);
                                    });
            if (itr2 == m_sites.end()) {
                LogE("override placement is wrong, rule: %s.", name.c_str());
                return false;
            }
            site = *itr2;
            if (guarantee && site->templateTag != "Invalid") {
                LogE("the site is already used.");
                return false;
            }
        } else {
            site = FindTargetForTemplate(templt, rule, guarantee);
            if (site != nullptr) {
                auto &centroid = site->polygon.Centroid();
                position = centroid + rule.overrideOffset;
            }
        }
        if (site != nullptr) {
            Rect bounds = templt.info.GetBounds(position, m_poiPadding);
            m_poiBounds.emplace_back(bounds);
            m_templates.emplace_back(position, &templt);
            UpdateNodeTags(*site, name, false);
            usedTemplates.emplace(name);
            maxCount--;
            minCount--;
        }
    }
    if (minCount > 0) {
        LogE("can not place all templates");
        return false;
    }
    return true;
}

void TemplateSpawning::SpawnTemplatesFromTemplateRules()
{
    std::vector<const TemplateSpawnRules *> rules;
    rules.reserve(32);
    for (auto item : m_world.worldTemplateRules2) {
        rules.push_back(item);
    }
    for (auto &subworldFile : m_world.subworldFiles2) {
        auto itr = m_settings.subworlds.find(subworldFile->name);
        for (auto &item : itr->second.subworldTemplateRules) {
            rules.push_back(&item);
        }
    }
    if (rules.empty()) {
        LogE("template rules is empty, check setting file.");
        return;
    }
    ArraySortHelper::Sort(
        rules, 0, (int)rules.size(),
        [](const TemplateSpawnRules *lhs, const TemplateSpawnRules *rhs) {
            return rhs->priority < lhs->priority;
        });
    std::set<std::string> usedTemplates;
    for (auto rule : rules) {
        for (int i = 0; i < rule->times; i++) {
            if (!ApplyTemplateRules(*rule, usedTemplates)) {
                break;
            }
        }
    }
}

static inline bool SitesContains(std::vector<Site *> &sites, float x, float y)
{
    for (auto site : sites) {
        if (site->polygon.Contains(x, y)) {
            return true;
        }
    }
    return false;
}

void TemplateSpawning::DrawWorldBorder()
{
    if (!GetDefaultData<bool>("DrawWorldBorder")) {
        return;
    }
    // bool force = GetDefaultData<bool>("DrawWorldBorderForce");
    int thickness = (int)GetDefaultData<float>("WorldBorderThickness");
    float range = GetDefaultData<float>("WorldBorderRange");
    float mapWidth = (int)m_world.worldsize.x;
    float mapHeight = (int)m_world.worldsize.y;
    float delta1 = 0;
    float delta2 = 0;
    float border1 = 0;
    float border2 = mapWidth - 1;
    KRandom rnd(0);
    std::vector<Site *> atLeft, atRight, atSurface, atDepths;
    for (auto site : m_sites) {
        if (site->tags.contains("RemoveWorldBorderOverVacuum")) {
            if (site->tags.contains("AtLeft")) {
                atLeft.push_back(site);
            }
            if (site->tags.contains("AtRight")) {
                atRight.push_back(site);
            }
            if (site->tags.contains("AtSurface")) {
                atSurface.push_back(site);
            }
            if (site->tags.contains("AtDepths")) {
                atDepths.push_back(site);
            }
        }
    }
    for (int y = mapHeight - 1; y >= 0; y--) {
        delta1 = std::max(-range, std::min(delta1 + rnd.Next(-2, 2), range));
        if (!SitesContains(atLeft, 1.0f, y)) {
            border1 = std::max(border1, thickness + delta1 - 1);
        }
        delta2 = std::max(-range, std::min(delta2 + rnd.Next(-2, 2), range));
        if (!SitesContains(atRight, mapWidth - 1.0f, y)) {
            border2 = std::min(border2, mapWidth - thickness - delta2);
        }
    }
    m_poiBounds.emplace_back(0.0f, 0.0f, border1 + 1, mapHeight);
    m_poiBounds.emplace_back(border2, 0.0f, mapWidth - border2, mapHeight);
    delta1 = delta2 = border1 = 0;
    border2 = mapHeight - 1;
    for (int x = 0; x < mapWidth; x++) {
        delta1 = std::max(-range, std::min(delta1 + rnd.Next(-2, 2), range));
        if (!SitesContains(atDepths, x, 1.0f)) {
            border1 = std::max(border1, thickness + delta1 - 1.0f);
        }
        delta2 = std::max(-range, std::min(delta2 + rnd.Next(-2, 2), range));
        if (!SitesContains(atSurface, x, mapHeight - 1.0f)) {
            border2 = std::min(border2, mapHeight - thickness - delta2);
        }
    }
    m_poiBounds.emplace_back(0.0f, 0.0f, mapWidth, border1 + 1.0f);
    m_poiBounds.emplace_back(0.0f, border2, mapWidth, mapHeight - border2);
    for (auto &rule : m_world.modifyLayoutTags) {
        for (auto &site : m_sites) {
            if (!DoesCellMatchFilters(*site, rule.allowedCellsFilter)) {
                continue;
            }
            for (auto &tag : rule.addTags) {
                site->tags.emplace(tag);
            }
            for (auto &tag : rule.removeTags) {
                site->tags.erase(tag);
            }
        }
    }
}

bool WorldGen::DetermineTemplates(std::vector<Site *> &sites, KRandom &random)
{
    TemplateSpawning spawning(random, m_world, m_settings);
    spawning.DetermineTemplatesForWorld(sites);
    m_templates.swap(spawning.GetTemplates());
    return true;
}
