#pragma once

#include "EditorBuildUtils.h"

class UWorld;

namespace McpLightingHandlers
{
#if WITH_EDITOR
bool RunLegacyLightingBuild(
    UWorld& World,
    ELightingBuildQuality Quality);
#endif
}
