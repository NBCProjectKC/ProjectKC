// Copyright Shared Orbit 2026. All Rights Reserved.

#include "MeshPaintingCoreModule.h"

#include "Interfaces/IPluginManager.h"
#include "ShaderCore.h"

#define LOCTEXT_NAMESPACE "FMeshPaintingCoreModule"

void FMeshPaintingCoreModule::StartupModule()
{
	const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("MeshPaintingCore"));
	if (Plugin.IsValid())
	{
		AddShaderSourceDirectoryMapping(
			TEXT("/MeshPaintingCore"),
			FPaths::Combine(Plugin->GetBaseDir(), TEXT("Shaders")));
	}
}

void FMeshPaintingCoreModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FMeshPaintingCoreModule, MeshPaintingCore)
