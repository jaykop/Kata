// Copyright (c) 2025 Rafael Valoto/Publisher. All rights reserved.
// Created for: WindowsDualsense_ds5w - Plugin to support DualSense controller on Windows.
// Planned Release Year: 2025

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SonyGamepadTouchProxy.generated.h"

/**
 *
 */
UCLASS(Blueprintable, BlueprintType)
class WINDOWSDUALSENSE_DS5W_API USonyGamepadTouchProxy : public UObject
{
	GENERATED_BODY()
public:
	/**
	 * Enables or disables the touch functionality on a specified DualSense controller.
	 *
	 * @param ControllerId The identifier of the controller for which the touch functionality should be enabled or disabled.
	 * @param bEnableTouch A boolean indicating whether to enable (true) or disable (false) the touch functionality.
	 */
	UFUNCTION(BlueprintCallable, Category = "SonyGamepadTouchProxy", meta = (DisplayName = "Enables Touch", ToolTip = "Touch X and Y position will only be enabled if gestures are disabled; if touch is enabled, you will only receive gestures."))
	static void EnableTouch(int32 ControllerId, bool bEnableTouch);

	/**
	 * Enables or disables gesture functionality on a specified DualSense controller.
	 *
	 * @param ControllerId The identifier of the controller for which the gesture functionality should be enabled or disabled.
	 * @param bEnableGesture A boolean indicating whether to enable (true) or disable (false) the gesture functionality.
	 */
	UFUNCTION(BlueprintCallable, Category = "SonyGamepadTouchProxy", meta = (DisplayName = "Enables Gesture", ToolTip = "A boolean indicating whether to enable (true) or disable (false)."))
	static void EnableGesture(int32 ControllerId, bool bEnableGesture);
};
