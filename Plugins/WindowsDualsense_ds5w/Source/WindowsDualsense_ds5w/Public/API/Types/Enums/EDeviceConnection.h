// Copyright (c) 2025 Rafael Valoto/Publisher. All rights reserved.
// Created for: WindowsDualsense_ds5w - Plugin to support DualSense controller on Windows.
// Planned Release Year: 2025

#pragma once

#include "CoreMinimal.h"
#include "EDeviceConnection.generated.h"

/**
 * @enum EDSDeviceType
 * Enum representing different types of devices.
 *
 * @value Default Represents the default type of device.
 * @value Edge Represents the Edge device type.
 * @value DualShock Represents the DualShock device type.
 */
UENUM(Blueprintable)
enum class EDeviceType : uint8
{
	DualSense UMETA(DisplayName = "DualSense Default"),
	DualSenseEdge UMETA(DisplayName = "DualSense Edge"),
	DualShock4 UMETA(DisplayName = "DualShock 4"),
	NotFound UMETA(DisplayName = "Not Found Device")
};

/**
 * @enum EDSDeviceConnection
 * Enum representing the types of connections for a device.
 *
 * @value Usb Represents a USB connection.
 * @value Bluetooth Represents a Bluetooth connection.
 * @value Unknown Represents an unknown or unrecognized connection type.
 */
UENUM(Blueprintable)
enum class EDeviceConnection : uint8
{
	Usb UMETA(DisplayName = "USB"),
	Bluetooth UMETA(DisplayName = "Bluetooth"),
	Unrecognized UMETA(DisplayName = "Unknown"),
};
