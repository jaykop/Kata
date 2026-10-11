// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class ProjectKataTarget : TargetRules
{
	public ProjectKataTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("ProjectKata");

		// 인게임 디버그 메뉴는 DeveloperTool 모듈이다. Test·Shipping에는 SlateIM과 함께 넣지 않는다.
		if (Configuration != UnrealTargetConfiguration.Shipping && Configuration != UnrealTargetConfiguration.Test)
		{
			ExtraModuleNames.Add("ProjectKataDebug");
		}
	}
}
