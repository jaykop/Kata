// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ProjectKata : ModuleRules
{
	public ProjectKata(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// 이 모듈은 Public/Private 디렉터리를 나누지 않으므로 모듈 루트를 인클루드 경로에 넣는다.
		// 이를 통해 "Testing/KataTestActor.h"처럼 모듈 루트 기준 경로를 사용할 수 있다.
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "GameplayTags", "GameplayAbilities", "KataConditions", "KataRuntime" });

		PrivateDependencyModuleNames.AddRange(new string[] {  });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
