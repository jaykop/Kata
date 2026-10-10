// Copyright (c) 2025 Rafael Valoto/Publisher. All rights reserved.
// Created for: WindowsDualsense_ds5w - Plugin to support DualSense controller on Windows.
// Planned Release Year: 2025

using System.IO;
using UnrealBuildTool;

public class WindowsDualsense_ds5w : ModuleRules
{
	public WindowsDualsense_ds5w(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(new string[]  {"Core", "CoreUObject", "Engine", "ApplicationCore", "InputCore", "InputDevice"});
		PrivateDependencyModuleNames.AddRange(new string[] {"Slate", "SlateCore"});
		PrivateDependencyModuleNames.AddRange(new string[] {"AudioMixer", "SignalProcessing", "AudioExtensions", "AudioPlatformConfiguration" });
		
		var gamepadCoreRoot = Path.Combine(ModuleDirectory, "Private", "GamepadCore");
		PublicIncludePaths.Add(Path.Combine(gamepadCoreRoot, "Source", "Public"));
		PrivateIncludePaths.Add(Path.Combine(gamepadCoreRoot, "Source", "Private"));
		
		
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			PublicSystemLibraries.Add("hid.lib");
			PublicSystemLibraries.Add("Cfgmgr32.lib");
		}
	}
}