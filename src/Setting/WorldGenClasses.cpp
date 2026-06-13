#include "ClusterLayout.hpp"
#include "TemplateContainer.hpp"

#include <algorithm>
#include <ranges>

#include "SettingsCache.hpp"
#include "Utils/KRandom.hpp"

template<typename T>
void BubbleSort(std::vector<T *> &list)
{
    size_t size = list.size();
    if (size < 2) {
        return;
    }
    size_t sorted = 0;
    while (sorted < size - 1) {
        size_t last = 0;
        for (size_t i = 1; i < size - sorted; i++) {
            auto &current = list[i];
            auto &previous = list[i - 1];
            if (*current < *previous) {
                std::swap(current, previous);
                last = i;
            }
        }
        sorted = size - last;
    }
}

void SubWorld::EnforceTemplateSpawnRuleSelfConsistency()
{
    if (subworldTemplateRules.empty()) {
        return;
    }
    for (auto &rule : subworldTemplateRules) {
        AllowedCellsFilter filter;
        filter.subworldNames.push_back(name);
        rule.allowedCellsFilter.emplace_back(filter);
    }
}

bool WorldTrait::IsValid(const World &world) const
{
    // int num = 0;
    int num2 = 0;
    for (auto &globalFeatureMod : globalFeatureMods) {
        // num += globalFeatureMod.second;
        num2 += (int)floor(world.worldTraitScale * globalFeatureMod.second);
    }
    if (globalFeatureMods.size() > 0 && num2 == 0) {
        LogE("Trait %s cannot be applied to world %s due to globalFeatureMods"
             " and worldTraitScale resulting in no features being generated.",
             name.c_str(), world.name.c_str());
        return false;
    }
    return true;
}

bool WorldTrait::ForbiddenSpaceOut() const
{
    return std::ranges::contains(forbiddenDLCIds, "EXPANSION1_ID");
}

static MixingConfig *FindSubworldMixing(SubworldMixingRule &rule,
                                        std::vector<MixingConfig *> &configs)
{
    std::vector<MixingConfig *> sorted = configs;
    BubbleSort(sorted);
    for (auto option : sorted) {
        if (option->maxCount <= 0) {
            continue;
        }
        auto setting = option->setting;
        bool flag = true;
        for (auto &forbiddenTag : rule.forbiddenTags) {
            auto itr = std::ranges::find(setting->mixingTags, forbiddenTag);
            if (itr != setting->mixingTags.end()) {
                flag = false;
                break;
            }
        }
        for (auto &requiredTag : rule.requiredTags) {
            auto itr = std::ranges::find(setting->mixingTags, requiredTag);
            if (itr == setting->mixingTags.end()) {
                flag = false;
                break;
            }
        }
        int minCount = std::max(rule.minCount, setting->subworld.minCount);
        int maxCount = std::min(rule.maxCount, setting->subworld.maxCount);
        if (minCount > maxCount) {
            flag = false;
        }
        if (flag) {
            return option;
        }
    }
    return nullptr;
}

void World::ApplayMixings(std::vector<MixingConfig *> &mixings)
{
    std::set<SubworldMixingRule *> appliedRules;
    for (auto &rule : subworldMixingRules) {
        if (mixings.empty()) {
            break;
        }
        MixingConfig *config = FindSubworldMixing(rule, mixings);
        if (config == nullptr) {
            continue;
        }
        auto setting = config->setting;
        auto &subworld = mixingSubworlds.emplace_back(setting->subworld);
        subworld.minCount = std::max(rule.minCount, subworld.minCount);
        subworld.maxCount = std::min(rule.maxCount, subworld.maxCount);
        subworldFiles2.push_back(&subworld);
        for (auto filter : unknownCellsAllowedSubworlds2) {
            auto filter2 = const_cast<AllowedCellsFilter *>(filter);
            filter2->Backup();
            for (auto &subname : filter2->subworldNames) {
                if (subname == rule.name) {
                    subname = subworld.name;
                }
            }
        }
        if (!appliedRules.contains(&rule)) {
            for (auto &item : setting->additionalWorldTemplateRules) {
                worldTemplateRules2.push_back(&item);
            }
            appliedRules.emplace(&rule);
        }
        config->maxCount--;
        config->minCount--;
        if (config->maxCount <= 0) {
            auto itr = std::ranges::find(mixings, config);
            mixings.erase(itr);
        }
    }
}

void World::ApplayTraits(const WorldTrait &trait, const SettingsCache &settings)
{
    startingPositionHorizontal2.Add(trait.startingBasePositionHorizontalMod);
    startingPositionVertical2.Add(trait.startingBasePositionVerticalMod);
    for (auto &subworld : trait.additionalSubworldFiles) {
        subworldFiles2.push_back(&subworld);
    }
    for (auto &filter : trait.additionalUnknownCellFilters) {
        unknownCellsAllowedSubworlds2.push_back(&filter);
    }
    BubbleSort(unknownCellsAllowedSubworlds2);
    for (auto &pair : trait.globalFeatureMods) {
        int count = (int)std::floor(worldTraitScale * pair.second);
        const Feature *feature = nullptr;
        for (auto &item : settings.traitFeatures) {
            if (item.type == pair.first) {
                feature = &item;
                break;
            }
        }
        if (feature == nullptr) {
            LogE("can not find feature %s", pair.first.c_str());
            continue;
        }
        for (int i = 0; i < count; i++) {
            globalFeatures2.emplace_back(feature);
        }
    }
    for (auto &ruleId : trait.removeWorldTemplateRulesById) {
        auto remove = std::ranges::remove_if(
            worldTemplateRules2, [&ruleId](const TemplateSpawnRules *rule) {
                return rule->ruleId == ruleId;
            });
        worldTemplateRules2.erase(remove.begin(), remove.end());
    }
    for (auto &rule : trait.additionalWorldTemplateRules) {
        worldTemplateRules2.push_back(&rule);
    }
}

void World::ClearMixingsAndTraits()
{
    startingPositionHorizontal2 = startingBasePositionHorizontal;
    startingPositionVertical2 = startingBasePositionVertical;
    subworldFiles2.clear();
    globalFeatures2.clear();
    mixingSubworlds.clear();
    mixingSubworlds.reserve(10);
    for (auto filter : unknownCellsAllowedSubworlds2) {
        auto filter2 = const_cast<AllowedCellsFilter *>(filter);
        filter2->Restore();
    }
    unknownCellsAllowedSubworlds2.clear();
    worldTemplateRules2.clear();
    subworldFiles2.reserve(subworldFiles.size());
    for (auto &subworld : subworldFiles) {
        subworldFiles2.push_back(&subworld);
    }
    unknownCellsAllowedSubworlds2.reserve(unknownCellsAllowedSubworlds.size());
    for (auto &filter : unknownCellsAllowedSubworlds) {
        unknownCellsAllowedSubworlds2.push_back(&filter);
    }
    worldTemplateRules2.reserve(worldTemplateRules.size());
    for (auto &rule : worldTemplateRules) {
        worldTemplateRules2.push_back(&rule);
    }
}
