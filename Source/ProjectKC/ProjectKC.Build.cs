// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ProjectKC : ModuleRules
{
	public ProjectKC(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PrivateIncludePaths.Add(ModuleDirectory);
	
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"GameplayAbilities",
			"GameplayTasks",
			
			// GMS
			"GameplayTags","GameplayMessageRuntime", "NetCore",

			// UI
			"CommonUI", "CommonInput", "UMG", "Slate", "SlateCore", "ModelViewViewModel", "DeveloperSettings",
			
			// Online & Steam Sessions
			"OnlineSubsystem", "OnlineSubsystemUtils", "OnlineSubsystemSteam",
			"AdvancedSessions", "AdvancedSteamSessions", "CoreOnline"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {  });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
