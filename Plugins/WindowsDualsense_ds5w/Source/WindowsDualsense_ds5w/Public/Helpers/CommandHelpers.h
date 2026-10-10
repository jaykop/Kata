// Copyright (c) 2025 Rafael Valoto/Publisher. All rights reserved.
// Created for: WindowsDualsense_ds5w - Plugin to support DualSense controller on Windows.
// Planned Release Year: 2025
#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"

class ISonyGamepadInterface;

/**
 * CommandHelpers centralizes all in-game console commands (ds.*) previously located in DualSenseLibrary.
 *
 * Usage examples (device-id first parameter):
 *  - ds.SetAudioByte <DeviceId> <Index 0-9> <Value 0-255>
 *  - ds.SetAudioLR <DeviceId> <L1> <L2> <R1> <R2> <Master>
 *  - ds.DumpAudioBytes <DeviceId>
 *  - ds.SetTrigR <DeviceId> <hex bytes...>
 *  - ds.SetTrigL <DeviceId> <hex bytes...>
 *  - ds.DumpTrig <DeviceId>
 *  - ds.ClearTrig <DeviceId>
 */
class WINDOWSDUALSENSE_DS5W_API FCommandHelpers
{
public:
	static void Register();

	// Handlers
	static void HandleSetAudioByte(const TArray<FString>& Args);
	static void HandleSetAudioLR(const TArray<FString>& Args);
	static void HandleDumpAudioBytes(const TArray<FString>& Args);
	static void HandleSetTrigR(const TArray<FString>& Args);
	static void HandleSetTrigL(const TArray<FString>& Args);
	static void HandleDumpTrig(const TArray<FString>& Args);
	static void HandleClearTrig(const TArray<FString>& Args);
	// Bow effect convenience commands
	static void HandleBowTrigR(const TArray<FString>& Args);
	static void HandleBowTrigL(const TArray<FString>& Args);
	// Galloping effect convenience commands
	static void HandleGallopTrigR(const TArray<FString>& Args);
	static void HandleGallopTrigL(const TArray<FString>& Args);

private:
	static bool ParseDeviceId(const TArray<FString>& Args, FInputDeviceId& OutDeviceId);
	static uint8 ClampByte(int32 V) { return static_cast<uint8>(FMath::Clamp(V, 0, 255)); }
	static bool ParseHexByte(const FString& Token, uint8& OutByte);
};
