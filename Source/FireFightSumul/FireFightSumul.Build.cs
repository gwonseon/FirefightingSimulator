// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class FireFightSumul : ModuleRules
{
	public FireFightSumul(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG" });
		PrivateDependencyModuleNames.Add("Niagara");
	}
}
