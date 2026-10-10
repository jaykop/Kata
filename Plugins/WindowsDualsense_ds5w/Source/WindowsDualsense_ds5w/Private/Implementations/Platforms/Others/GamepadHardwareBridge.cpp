// Copyright (c) 2025 Rafael Valoto/Publisher. All rights reserved.
// Created for: WindowsDualsense_ds5w - Plugin to support DualSense controller on Windows.
// Planned Release Year: 2025

#include "Implementations/Platforms/Others/GamepadHardwareBridge.h"
#include "Helpers/DualSenseLog.h"
#include "Implementations/Adapters/DeviceRegistry.h"

void FGamepadHardwareBridge::InjectHardwarePlatform(std::unique_ptr<IPlatformHardwareInfo> InPlatform)
{
	if (InPlatform)
	{
		// Initialize PlatformHardware, (e.g., FMacHardware, FSonyHardware)
		IPlatformHardwareInfo::SetInstance(std::move(InPlatform));

		// Re-Initialize devices
		FDeviceRegistry::Initialize();
		UE_LOG(LogDualSense, Log, TEXT("GamepadCore: Platform swapped and Registry re-initialized."));
	}
}
