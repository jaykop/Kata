// Copyright (c) 2025 Rafael Valoto/Publisher. All rights reserved.
// Created for: WindowsDualsense_ds5w - Plugin to support DualSense controller on Windows.
// Planned Release Year: 2025

#pragma once

#include "API/Types/Enums/EDeviceCommons.h"
#include "API/Types/Enums/EDeviceConnection.h"
#include "CoreMinimal.h"
#include "Types/DualSenseFeatureReport.h"
#include "UObject/Object.h"
#include "SonyGamepadBaseProxy.generated.h"

/**
 * Proxy class for interacting with Sony gamepad devices such as DualSense or DualShock controllers.
 * This class provides static methods to handle various functionalities, including device status,
 * connection type, battery levels, LED effects, motion sensor calibration, touch capabilities,
 * and deprecation of certain methods.
 */
UCLASS(Blueprintable, BlueprintType)
class WINDOWSDUALSENSE_DS5W_API USonyGamepadBaseProxy : public UObject
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "SonyGamepad Status", meta = (DisplayName = "Update Output", ToolTip = ""))
	static void UpdateOutput(int32 ControllerId);
	/**
	 * Checks if the DualSense or DualShock device with the specified Controller ID is connected.
	 *
	 * @param ControllerId The ID of the controller to check for connectivity.
	 * @return True if the DualSense or DualShock  device is connected, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "SonyGamepad Status", meta = (DisplayName = "Device Is Connected", ToolTip = "Checks if the device with the specified Controller ID is connected."))
	static bool DeviceIsConnected(int32 ControllerId);
	/**
	 * Retrieves the device type of a connected gamepad based on the provided controller ID.
	 *
	 * @param ControllerId The ID of the controller to query for its device type.
	 * @return The type of the device connected (e.g., DualSense, DualShock4, or NotFound) as an EDeviceType.
	 */
	UFUNCTION(BlueprintCallable, Category = "SonyGamepad Status", meta = (DisplayName = "Device Type (e.g., DualSense)", ToolTip = "The type of the device connected (e.g., DualSense, DualShock4, or NotFound) as an EDeviceType."))
	static EDeviceType GetDeviceType(int32 ControllerId);
	/**
	 * Retrieves the connection type of the DualSense or DualShock device with the specified Controller ID.
	 *
	 * @param ControllerId The ID of the controller whose connection type is being queried.
	 * @return The connection type of the device, which can be USB, Bluetooth, or Unrecognized.
	 */
	UFUNCTION(BlueprintCallable, Category = "SonyGamepad Status", meta = (DisplayName = "Connection Type (e.g., USB, Bluetooth, Unknown)"))
	static EDeviceConnection GetConnectionType(int32 ControllerId);
	/**
	 * Retrieves the battery level of the DualSense or DualShock controller for the specified controller ID.
	 *
	 * This method uses the DualSense or DualShock library instance associated with the provided controller ID
	 * to fetch the battery level. If the library instance is not found, the method returns 0.0f.
	 *
	 * @param ControllerId The ID of the DualSense or DualShock controller to query.
	 * @return The battery level of the controller as a float. Returns 0.0f if the library instance cannot be retrieved.
	 */
	UFUNCTION(BlueprintCallable, Category = "SonyGamepad Status", meta = (DisplayName = "Battery Level (0.0f-100.0f)"))
	static float BatteryLevelDevice(int32 ControllerId);
};
