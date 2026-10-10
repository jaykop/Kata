// Copyright (c) 2025 Rafael Valoto/Publisher. All rights reserved.
// Created for: WindowsDualsense_ds5w - Plugin to support DualSense controller on Windows.
// Planned Release Year: 2025

#pragma once

#include "CoreMinimal.h"
#include "Types/DualSenseFeatureReport.h"
#include "UObject/Object.h"
#include "SonyGamepadSettingsProxy.generated.h"
namespace PluginSettings
{
	inline float PollInterval = 0.0166f;
	inline float MadgwickBeta = 0.1f;
} // namespace PluginSettings
/**
 *
 */
UCLASS(Blueprintable, BlueprintType)
class WINDOWSDUALSENSE_DS5W_API USonyGamepadSettingsProxy : public UObject
{
	GENERATED_BODY()
public:
	/**
	 * Temporary
	 */
	UFUNCTION(BlueprintCallable, Category = "SonyGamepadSettingsProxy", meta = (DisplayName = "DualSense Settings"))
	static void DualSenseSettings(int32 ControllerId, FDualSenseFeatureReport Value);

	/**
	 * Adjusts the Madgwick filter's beta parameter, determining the trade-off between noise suppression and response speed.
	 *
	 * @param Value The beta value to set for the filter. Larger values increase response speed but may introduce noise, while smaller values suppress noise at the cost of slower response.
	 */
	UFUNCTION(BlueprintCallable, Category = "SonyGamepadSettingsProxy", meta = (DisplayName = "Madgwick Beta", ToolTip = "It should be chosen based on the desired trade-off between noise suppression and response speed."))
	static void MadgwickBeta(float Value);

	/**
	 * Adjusts the Madgwick filter's beta parameter, determining the trade-off between noise suppression and response speed.
	 *
	 * @param Value The beta value to set for the filter. Larger values increase response speed but may introduce noise, while smaller values suppress noise at the cost of slower response.
	 */
	UFUNCTION(BlueprintCallable, Category = "SonyGamepadSettingsProxy", meta = (DisplayName = "Tock Poll Interval", ToolTip = "Defines the interval, in seconds, between periodic polling operations."))
	static void PollInterval(float Value);
};
