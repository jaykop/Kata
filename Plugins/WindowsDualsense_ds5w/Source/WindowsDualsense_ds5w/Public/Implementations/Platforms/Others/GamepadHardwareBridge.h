// Copyright (c) 2025 Rafael Valoto/Publisher. All rights reserved.
// Created for: WindowsDualsense_ds5w - Plugin to support DualSense controller on Windows.
// Planned Release Year: 2025

#pragma once
#include "GCore/Interfaces/IPlatformHardwareInfo.h"

class WINDOWSDUALSENSE_DS5W_API FGamepadHardwareBridge
{
	/**
	 * Injects a hardware platform into the gamepad hardware bridge. This method
	 * sets the platform hardware instance and reinitializes the device registry to
	 * accommodate the injected platform.
	 *
	 * @param InPlatform A unique pointer to the platform-specific hardware information
	 *                   to be injected (e.g., FMacHardware, FSonyHardware). It must
	 *                   not be null.
	 */
public:
	static void InjectHardwarePlatform(std::unique_ptr<IPlatformHardwareInfo> InPlatform);
};
