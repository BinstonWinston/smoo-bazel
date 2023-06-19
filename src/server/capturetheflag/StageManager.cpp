#include "server/capturetheflag/StageManager.hpp"

#include "al/util.hpp"
#include "sead/container/seadSafeArray.h"

struct SubAreaEntranceLocation {
    SubAreaEntranceLocation(const char* parentStageName, StageData const& stageData, sead::Vector3f const& entrancePosition)
        : parentStageName(parentStageName)
        , stageData(stageData)
        , entrancePosition(entrancePosition)
        {}
    const char* parentStageName;
    StageData stageData;
    sead::Vector3f entrancePosition;
};

bool operator==(StageData const& lhs, StageData const& rhs) {
    if (lhs.scenarioNum < 15) {
        return al::isEqualString(lhs.stageName, rhs.stageName);
    }
    
    return al::isEqualString(lhs.stageName, rhs.stageName) && lhs.scenarioNum == rhs.scenarioNum;
}

std::optional<std::pair<StageData, const char*>> StageManager::getHomeTeleport(StageData const& inStageData, FlagTeam team) {
    const auto HOME_LOCATIONS = sead::toArray({
        // Cascade
        std::make_tuple("WaterfallWorldHomeStage", FlagTeam::RED, "CapAppearExExit"),
        std::make_tuple("WaterfallWorldHomeStage", FlagTeam::BLUE, ""),
    });

    auto stageData = inStageData;
    if (StageManager::isSubArea(stageData)) {
        auto const parentStageName = StageManager::getParentStageName(stageData);
        if (!parentStageName.has_value()) {
            return {}; // No home teleport specified for a top-level stage. This indicates a kingdom is missing from the home location list (or a sub-area is missing from the list associated to a kingdom).
        }
        stageData = StageData(parentStageName.value(), -1);
    }

    for (auto const& homeLocation: HOME_LOCATIONS) {
        if (al::isEqualString(std::get<0>(homeLocation), stageData.stageName) && (std::get<1>(homeLocation) == team)) {
            return std::make_pair(stageData, std::get<2>(homeLocation));
        }
    }
    return {};
}


std::optional<BorderTransform> StageManager::getBorderTransform(StageData const& stageData) {
    const auto BORDER_TRANSFORMS = sead::toArray({
        // Cascade
        std::make_pair("WaterfallWorldHomeStage", BorderTransform({-1441.66f, 2602.f - 50000.f, 440.f})),
    });

    for (auto const& borderTransform: BORDER_TRANSFORMS) {
        if (al::isEqualString(borderTransform.first, stageData.stageName)) {
            return borderTransform.second;
        }
    }
    return {};
}

sead::Vector3f StageManager::replacePositionWithSubAreaEntranceIfInSubArea(sead::Vector3f const& pos, StageData const& stageData) {
    return getSubAreaEntranceLocation(stageData).value_or(pos);
}


namespace {

// To find stage entrance positions, find the stages map .byml, e.g. WaterfallWorldHomeStageMap.byml (in StageData/WaterfallWorldHomeStageMap.szs)
// Then convert the byml to yaml, and search for the regex "ChangeStageName: [A-Z]" (just needs a capital letter for the value to rule out "null").
// Then the objects you find are the stage entrances, scroll down a tiny bit to see the position (Translate: {X: ..., Y: ..., Z: ...})
sead::SafeArray<SubAreaEntranceLocation, 5> getSubAreas() {
    const auto SUB_AREAS = sead::toArray({
        // Cascade
        SubAreaEntranceLocation("WaterfallWorldHomeStage", {"WindBlowExStage", 0}, sead::Vector3f(-388.9088134765625, 1650.0, 4419.41748046875)),
        SubAreaEntranceLocation("WaterfallWorldHomeStage", {"WanwanClashExStage", 0}, sead::Vector3f(-10394.46875, 3590.0, -919.23876953125)),
        SubAreaEntranceLocation("WaterfallWorldHomeStage", {"Lift2DExStage", 0}, sead::Vector3f(1148.769287109375, 2750.0, -3552.932373046875)),
        SubAreaEntranceLocation("WaterfallWorldHomeStage", {"TrexPoppunExStage", 0}, sead::Vector3f(4702.259765625, 2250.0, 8237.7939453125)),
        SubAreaEntranceLocation("WaterfallWorldHomeStage", {"CapAppearExStage", 0}, sead::Vector3f(-5374.01123046875, 3267.029296875, -1484.9241943359375)),
    });
    return SUB_AREAS;
}

std::optional<SubAreaEntranceLocation> searchForSubAreaData(StageData const& stageData) {
    for (auto const& subArea: getSubAreas()) {
        if (stageData == subArea.stageData) {
            return subArea;
        }
    }
    return {};
}

}

size_t StageManager::getSubAreaCount() {
    return getSubAreas().size();
}

const char* StageManager::getSubAreaName(size_t index) {
    return getSubAreas()[index].stageData.stageName;
}

sead::Vector3f StageManager::getSubAreaLocation(size_t index) {
    return getSubAreas()[index].entrancePosition;
}

std::optional<const char*> StageManager::getParentStageName(StageData const& stageData) {
    auto const subAreaData = searchForSubAreaData(stageData);
    if (!subAreaData.has_value()) {
        return {};
    }
    return subAreaData.value().parentStageName;
}


bool StageManager::isSubArea(StageData const& stageData) {
    return searchForSubAreaData(stageData).has_value();
}

std::optional<sead::Vector3f> StageManager::getSubAreaEntranceLocation(StageData const& stageData) {
    auto const subAreaData = searchForSubAreaData(stageData);
    if (!subAreaData.has_value()) {
        return {};
    }
    return subAreaData.value().entrancePosition;
}
