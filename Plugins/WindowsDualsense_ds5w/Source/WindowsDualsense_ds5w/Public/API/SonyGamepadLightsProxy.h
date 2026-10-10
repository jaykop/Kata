// Copyright (c) 2025 Rafael Valoto/Publisher. All rights reserved.
// Created for: WindowsDualsense_ds5w - Plugin to support DualSense controller on Windows.
// Planned Release Year: 2025

#pragma once

#include "API/Types/Enums/EDeviceCommons.h"
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SonyGamepadLightsProxy.generated.h"

/**
 *
 */
UCLASS()
class WINDOWSDUALSENSE_DS5W_API USonyGamepadLightsProxy : public UObject
{
	GENERATED_BODY()
public:
	/**
	 * Sets the lightbar color on a controller.
	 *
	 * @param ControllerId The ID of the DualSense controller to be affected.
	 * @param Color The desired lightbar color, represented as an FColor value.
	 */
	UFUNCTION(BlueprintCallable, Category = "SonyGamepadLights", meta = (AdvancedDisplay = "Lightbar Color (R,G,B)", ToolTip = "Sets the lightbar color on a controller."))
	static void Lightbar(int32 ControllerId, FColor Color);
	/**
	 * Sets the lightbar to flash with specified color and timing on a DualShock4 controller.
	 *
	 * @param ControllerId The ID of the DualSense controller to be affected.
	 * @param Color The desired lightbar color, represented as an FColor value.
	 * @param BrightnessTime The duration of brightness during the flash cycle, in seconds. Defaults to 0.0f.
	 * @param ToggleTime The interval between lightbar flashes, in seconds. Defaults to 0.0f.
	 */
	UFUNCTION(BlueprintCallable, Category = "SonyGamepadLights", meta = (AdvancedDisplay = "Lightbar Flash (DualShock4)", ToolTip = "Sets the flash lightbar toogle."))
	static void LightbarFlash(int32 ControllerId, FColor Color, float BrightnessTime = 0.0f, float ToggleTime = 0.0f);
	/**
	 * Configures the player LED on a DualSense controller.
	 *
	 * @param ControllerId The ID of the DualSense controller to be affected.
	 * @param Value Specifies which player LED(s) should be activated, defined by the ELedPlayerEnum.
	 * @param Brightness The brightness level of the player LED, represented as an ELedBrightnessEnum.
	 */
	UFUNCTION(BlueprintCallable, Category = "SonyGamepadLights", meta = (AdvancedDisplay = "Player Led (e.g., One, Two)"))
	static void PlayerLed(int32 ControllerId, ELedPlayerEnum Value, ELedBrightnessEnum Brightness);
};
