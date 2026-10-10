// Copyright (c) 2025 Rafael Valoto/Publisher. All rights reserved.
// Created for: WindowsDualsense_ds5w - Plugin to support DualSense controller on Windows.
// Planned Release Year: 2025

#pragma once

#include "API/Types/Enums/EDeviceCommons.h"
#include "CoreMinimal.h"
#include "DualSenseFeatureReport.generated.h"

/**
 * Represents the DualSense Feature Report struct, managing various
 * configurable settings and states for a DualSense controller.
 *
 * This struct provides properties to control the following features:
 * - Microphone status
 * - Headset audio
 * - Speaker settings
 * - Vibration mode
 * - Microphone and audio volume levels
 * - Soft rumble reduction intensity
 * - Adaptive trigger softness level
 *
 * Each property is categorized under "DualSense Settings" and is editable both in the editor
 * and readable/writable in Blueprints. The constructor initializes these properties
 * with default values for convenient usage.
 *
 * Category: DualSense Settings
 */
USTRUCT(BlueprintType)
struct FDualSenseFeatureReport
{
	GENERATED_BODY()

	/**
	 * Represents the microphone status of the DualSense controller audio settings.
	 * This variable utilizes the EDualSenseAudioFeatureReport enum to toggle the microphone feature.
	 * The microphone can be toggled On or Off, influencing the device's audio configuration.
	 *
	 * Category: DualSense Settings
	 * Blueprint Read/Write: Enabled
	 * Possible Values:
	 *   - EDualSenseAudioFeatureReport::On: Enables the microphone.
	 *   - EDualSenseAudioFeatureReport::Off: Disables the microphone.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DualSense Settings", meta = (ToolTip = "Enables the microphone.", DisplayName = "Microphone"))
	EDualSenseAudioFeatureReport MicStatus;
	/**
	 * Specifies the operational state of the audio headset on a DualSense controller.
	 * The variable can be configured to enable or disable the headset audio functionality.
	 *
	 * Category:
	 * - DualSense Settings
	 *
	 * Access:
	 * - Editable anywhere in the editor.
	 * - Readable and writable in Blueprints.
	 *
	 * Enum:
	 * - EDualSenseAudioFeatureReport::On: Enables headset audio.
	 * - EDualSenseAudioFeatureReport::Off: Disables headset audio.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DualSense Settings", meta = (ToolTip = "Enables headset audio.", DisplayName = "Headset Audio"))
	EDualSenseAudioFeatureReport AudioHeadset;
	/**
	 * Represents the audio speaker settings for a DualSense device.
	 *
	 * This variable allows enabling or disabling the speaker functionality
	 * on the DualSense device. It uses the EDualSenseAudioFeatureReport
	 * enum to specify the state of the speaker, either "On" or "Off".
	 *
	 * Category: DualSense Settings
	 * Access: Readable and writable in Blueprints
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DualSense Settings", meta = (ToolTip = "Enables the speaker.", DisplayName = "Speaker Audio"))
	EDualSenseAudioFeatureReport AudioSpeaker;
	/**
	 * The microphone volume level for the DualSense device.
	 *
	 * This variable determines the intensity of the microphone input, allowing users
	 * to adjust the microphone sensitivity or volume output. The value is clamped between 0 and 100.
	 *
	 * - Category: DualSense Settings
	 * - Range: 0 (minimum volume) to 100 (maximum volume)
	 * - Metadata:
	 *   - ClampMin: 0
	 *   - ClampMax: 100
	 *   - UIMin: 0
	 *   - UIMax: 100
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DualSense Settings",
	          meta = (ClampMin = "0", ClampMax = "100", UIMin = "0", UIMax = "100", ToolTip = "Adjusts the microphone volume level.", DisplayName = "Microphone Volume"))
	int32 MicVolume;
	/**
	 * Represents the audio volume setting for a DualSense device.
	 * This value determines the volume level applied to the audio output, including the headset and speaker.
	 *
	 * - The valid range for this property is from 0 to 100, where:
	 *   - 0 represents no audio output (muted).
	 *   - 100 represents the maximum audio output volume.
	 * - Values outside this range are clamped automatically.
	 *
	 * This property can be edited in the editor or accessed in Blueprints. It appears under the "DualSense Settings" category in the editor.
	 *
	 * Metadata:
	 * - ClampMin: 0 (Minimum allowable value for automatic clamping)
	 * - ClampMax: 100 (Maximum allowable value for automatic clamping)
	 * - UIMin: 0 (Minimum value shown in UI sliders)
	 * - UIMax: 100 (Maximum value shown in UI sliders)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DualSense Settings",
	          meta = (ClampMin = "0", ClampMax = "100", UIMin = "0", UIMax = "100", ToolTip = "Adjusts the audio output volume level.", DisplayName = "Audio Volume"))
	int32 AudioVolume;

	/**
	 * Specifies the vibration mode for the DualSense device. Controls the type of haptic feedback
	 * or vibration effect to be applied on the controller using the available feature report.
	 *
	 * The possible values are:
	 * - EDualSenseDeviceFeatureReport::Off: Disables vibration feedback.
	 * - EDualSenseDeviceFeatureReport::DefaultRumble: Enables default rumble feedback.
	 * - EDualSenseDeviceFeatureReport::HapticSoftRumble: Enables soft haptic rumble feedback.
	 *
	 * This property can be edited within the editor and accessed in Blueprints, allowing advanced customization
	 * of the DualSense controller's haptic behavior to provide a tailored user experience.
	 *
	 * Category: DualSense Settings
	 *
	 * Display Name: Soft haptic feedback, advanced vibrate
	 */
	UPROPERTY(
	    EditAnywhere,
	    BlueprintReadWrite,
	    Category = "DualSense Settings",
	    meta = (DisplayName = "Vibration mode",
	            ToolTip = "Advanced vibration that are directly generated from real-time audio analysis for a more immersive haptic experience."))
	EDualSenseDeviceFeatureReport VibrationMode;
	/**
	 * Determines the reduction intensity of the soft rumble effect for a DualSense device.
	 *
	 * The value represents the level of adjustment applied to the soft haptic feedback vibration,
	 * ranging from 0 (no reduction) to 15 (maximum reduction).
	 *
	 * The value is clamped and displayed in the UI with a minimum of 0 and a maximum of 15.
	 *
	 * Category:
	 * - DualSense Settings
	 *
	 * Metadata:
	 * - ClampMin: 0
	 * - ClampMax: 15
	 * - UIMin: 0
	 * - UIMax: 15
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DualSense Settings",
	          meta = (ClampMin = "0", ClampMax = "15", UIMin = "0", UIMax = "15", ToolTip = "Adjusts the soft rumble effect intensity.", DisplayName = "Soft Rumble Reduction"))
	int32 SoftRumbleReduce;
	/**
	 * Specifies the softness level of the adaptive triggers on a DualSense controller.
	 *
	 * This property allows fine-tuning of the trigger pressure resistance, ranging from "Off" to "Very Soft".
	 * Users can adjust the value in both the editor and at runtime via Blueprints.
	 *
	 * Valid range:
	 * - Minimum: 0 (Off)
	 * - Maximum: 15 (Very Soft)
	 *
	 * The property is clamped between 0 and 15, and the UI provides a slider ranging between these values
	 * for easier configuration.
	 *
	 * Category: DualSense Settings
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DualSense Settings",
	          meta = (ClampMin = "0", ClampMax = "15", UIMin = "0", UIMax = "15", ToolTip = "Adjusts the trigger pressure resistance.", DisplayName = "Trigger Softness Level"))
	EDualSenseTriggerSoftnessLevel TriggerSoftnessLevel;

	/**
	 * Default constructor for the FDualSenseFeatureReport struct.
	 * Initializes various audio and vibration settings for the DualSense controller.
	 *
	 * The constructor sets default states for microphone, audio outputs, vibration, and trigger softness levels.
	 *
	 * @return A default-initialized instance of FDualSenseFeatureReport.
	 */
	FDualSenseFeatureReport()
	    : MicStatus(EDualSenseAudioFeatureReport::Off)
	    , AudioHeadset(EDualSenseAudioFeatureReport::Off)
	    , AudioSpeaker(EDualSenseAudioFeatureReport::On)
	    , MicVolume(0)
	    , AudioVolume(0)
	    , VibrationMode(EDualSenseDeviceFeatureReport::DefaultRumble)
	    , SoftRumbleReduce(0)
	    , TriggerSoftnessLevel(EDualSenseTriggerSoftnessLevel::Medium)
	{
	}
};
