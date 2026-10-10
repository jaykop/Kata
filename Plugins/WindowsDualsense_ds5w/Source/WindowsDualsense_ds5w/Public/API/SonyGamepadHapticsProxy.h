// Copyright (c) 2025 Rafael Valoto/Publisher. All rights reserved.
// Created for: WindowsDualsense_ds5w - Plugin to support DualSense controller on Windows.
// Planned Release Year: 2025

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SonyGamepadHapticsProxy.generated.h"

/**
 *
 */
UCLASS(Blueprintable, BlueprintType)
class WINDOWSDUALSENSE_DS5W_API USonyGamepadHapticsProxy : public UObject
{
	GENERATED_BODY()
public:
	/**
	 * @brief Registers a specific audio submix for a DualSense controller.
	 *
	 * Links a given sound submix to a connected PlayStation DualSense controller, identified by ControllerId.
	 * Enables audio haptics functionality to synchronize sound experiences with the controller's haptics capability.
	 *
	 * @param ControllerId The identifier for the connected DualSense controller.
	 * @param Submix The audio submix to be registered for haptic feedback on the controller.
	 */
	UFUNCTION(BlueprintCallable, Category = "SonyGamepadHaptics", meta = (DisplayName = "Register Submix for devices haptics"))
	static void RegisterSubmixForDevice(int32 ControllerId, USoundSubmix* Submix);
	/**
	 * @brief Unregisters a submix listener associated with the specified DualSense controller device.
	 *
	 * This function removes the haptics listener for the specified controller ID, ensuring
	 * that any previously registered submix for audio or haptic feedback is no longer active.
	 *
	 * @param ControllerId The ID of the DualSense controller for which the submix listener will be unregistered.
	 */
	UFUNCTION(BlueprintCallable, Category = "SonyGamepadHaptics", meta = (DisplayName = "Unregister Submix for devices haptics"))
	static void UnregisterSubmixForDevice(int32 ControllerId);
};
