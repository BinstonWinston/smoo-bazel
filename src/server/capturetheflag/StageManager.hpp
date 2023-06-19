#pragma once

#include <optional>
#include <tuple>

#include "sead/math/seadVector.hpp"
#include "sead/prim/seadSafeString.hpp"

#include "server/capturetheflag/FlagTeam.hpp"

struct StageData {
    const char* stageName;
    u8 scenarioNum;
};

bool operator==(StageData const& lhs, StageData const& rhs);

struct BorderTransform {
    sead::Vector3f position;
    // TODO add rotation
};

class StageManager {
public: // This block is for debugging only, not needed for actual functionality
    static size_t getSubAreaCount();
    static const char* getSubAreaName(size_t index);
    static sead::Vector3f getSubAreaLocation(size_t index);

public:
    static std::optional<std::pair<StageData, const char*>> getHomeTeleport(StageData const& stageData, FlagTeam team);

    static std::optional<BorderTransform> getBorderTransform(StageData const& stageData);

    static std::optional<const char*> getParentStageName(StageData const& stageData);

    static bool isSubArea(StageData const& stageData);

    /**
     * @brief If not in a sub area, simply returns the given position. If in a sub-area, returns the position of the entrance (e.g. moon pipe or door) to the sub-area
     *  in the coordinates of the parent stage map.
     * 
     * @param pos
     * @param stageData 
     * @return sead::Vector3f If not in a sub area, returns the original position. If in a sub-area, returns the position of the entrance (e.g. moon pipe or door) to the sub-area
     *  in the coordinates of the parent stage map.
     */
    static sead::Vector3f replacePositionWithSubAreaEntranceIfInSubArea(sead::Vector3f const& pos, StageData const& stageData);

private:
    /**
     * @brief Get the sub area entrance location (e.g. moon pipe or door) in the coordinates of the parent stage map
     * 
     * @param stageData
     * @return std::optional<sead::Vector3f> 
     */
    static std::optional<sead::Vector3f> getSubAreaEntranceLocation(StageData const& stageData);
};