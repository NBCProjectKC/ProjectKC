// Copyright Shared Orbit 2026. All Rights Reserved.
using UnrealBuildTool;

public class MeshPaintingCore : ModuleRules
{
	public MeshPaintingCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		PrivatePCHHeaderFile = "Private/MeshPaintingCorePrivatePCH.h";
		PrecompileForTargets = PrecompileTargetsType.Any;
		CppCompileWarningSettings.DeprecationWarningLevel = WarningLevel.Off;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"EnhancedInput",
				"NetCore",
				"UMG",
				"Slate",
				"SlateCore"
			});

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"ApplicationCore",
				"ImageCore",
				"InputCore",
				"Projects",
				"RenderCore",
				"RHI"
			});
	}
}
