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
		// GamepadCore의 miniaudio_impl.cpp는 miniaudio.h 구현부에서 <windows.h>를 WIN32_LEAN_AND_MEAN 없이 포함한다.
		// unity 빌드로 묶이면 PlaySound 같은 Windows 매크로가 뒤에 오는 오디오 헤더(EQuartzCommandType::PlaySound)를 깨뜨린다.
		// 벤더 코드의 오디오 백엔드 구성을 바꾸지 않도록 이 모듈만 unity 빌드를 끈다.
		bUseUnity = false;
		
		PublicDependencyModuleNames.AddRange(new string[]  {"Core", "CoreUObject", "Engine", "ApplicationCore", "InputCore", "InputDevice"});
		// 원본의 Slate·SlateCore 의존은 Linux·Mac 입력 전처리기용이었다. 그 경로를 삭제해 의존을 뺐다.
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