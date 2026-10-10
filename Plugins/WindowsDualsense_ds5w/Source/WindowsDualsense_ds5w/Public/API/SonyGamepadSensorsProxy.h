// Copyright (c) 2025 Rafael Valoto/Publisher. All rights reserved.
// Created for: WindowsDualsense_ds5w - Plugin to support DualSense controller on Windows.
// Planned Release Year: 2025

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SonyGamepadSensorsProxy.generated.h"

/**
 *
 */
UCLASS(Blueprintable, BlueprintType)
class WINDOWSDUALSENSE_DS5W_API USonyGamepadSensorsProxy : public UObject
{
	GENERATED_BODY()
public:
	/**
	 * Resets the gyroscope orientation for the specified controller.
	 * This can be used to recalibrate the orientation when the controller's current orientation
	 * does not align with the expected neutral position.
	 *
	 * @param ControllerId The ID of the controller whose gyroscope orientation should be reset.
	 */
	UFUNCTION(BlueprintCallable, Category = "SonyGamepadSensors", meta = (AdvancedDisplay = "Reset Gyroscope Orientation", ToolTip = "Resets the gyroscope orientation for the specified controller. This can be used to recalibrate the orientation when the controller's current orientation does not align with the expected neutral position."))
	static void ResetGyroOrientation(int32 ControllerId);
	/**
	 * Enables or disables the gyroscope functionality for a specified controller.
	 *
	 * @param ControllerId The ID of the controller for which the gyroscope functionality is to be modified.
	 * @param bEnableGyroscope Set to true to enable the gyroscope, or false to disable it.
	 */
	UFUNCTION(BlueprintCallable, Category = "SonyGamepadSensors", meta = (ToolTip = "Enables or disables the gyroscope functionality for a specified controller."))
	static void EnableGyroscopeValues(int32 ControllerId, bool bEnableGyroscope);

	/**
	 * Deprecated v1.2.20
	 * Initiates the motion sensor calibration process for the specified controller.
	 * The calibration adjusts the motion sensor sensitivity and dead zone settings.
	 *
	 * @param ControllerId The ID of the controller to be calibrated.
	 * @param Duration The duration of the calibration process in seconds.
	 * @param DeadZone The sensitivity threshold below which motion input will be ignored.
	 */
	UFUNCTION(BlueprintCallable, Category = "SonyGamepadSensors", meta = (AdvancedDisplay = "Calibration Duration (Legacy)", ToolTip = "Deprecated v1.2.20"))
	static void StartMotionSensorCalibration(
	    int32 ControllerId,
	    UPARAM(DisplayName = "Calibration Duration (Seconds)", meta = (ClampMin = "1.0", ClampMax = "10.0", UIMin = "1.0", UIMax = "10.0",
	                                                                   ToolTip = "The time in seconds to collect sensor data for calculating the stable center (baseline). Longer durations can provide a more accurate baseline.")) float Duration = 2.0f,
	    UPARAM(DisplayName = "Noise Deadzone Percentage", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0",
	                                                              ToolTip = "A percentage (0.0 to 1.0) of the sensor noise range to ignore after calibration. A higher value creates a larger deadzone, filtering out more residual noise but potentially ignoring very subtle movements.")) float DeadZone = 0.5f)
	{}
	/**
	 * Deprecated v1.2.20
	 * Retrieves the calibration status of the motion sensor for the specified controller.
	 *
	 * @param ControllerId The ID of the controller whose motion sensor calibration status is being queried.
	 * @param Progress A reference to a variable where the calibration progress will be stored, expressed as a percentage.
	 * @return True if the calibration process is in progress, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "SonyGamepadSensors (Legacy)", meta = (ToolTip = "Deprecated v1.2.20"))
	static bool GetMotionSensorCalibrationStatus(int32 ControllerId, float& Progress) { return false; }
};
