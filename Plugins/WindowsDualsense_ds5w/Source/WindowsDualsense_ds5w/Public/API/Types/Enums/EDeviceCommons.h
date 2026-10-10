// Copyright (c) 2025 Rafael Valoto/Publisher. All rights reserved.
// Created for: WindowsDualsense_ds5w - Plugin to support DualSense controller on Windows.
// Planned Release Year: 2025

#pragma once

#include "CoreMinimal.h"
#include "GCore/Types/DSCoreTypes.h"
#include "EDeviceCommons.generated.h"

/**
 * @brief Enum class representing various LED microphone states.
 *
 * This enum class is used to define the states for LED microphone controls on a compatible device.
 *
 * Enum values:
 * - MicOff: Represents the "Mic Off" state.
 * - MicOn: Represents the "Mic On" state.
 * - Pulse: Represents the "Pulse" state.
 */
UENUM(BlueprintType)
enum class ELedMicEnum : uint8
{
	MicOn = 0,
	MicOff = 1,
	Pulse = 2
};

/**
 * @enum ELedPlayerEnum
 * Enumeration for controlling DualSense LED light player effects.
 */
UENUM(BlueprintType)
enum class ELedPlayerEnum : uint8
{
	Off = 0 UMETA(DisplayName = "Led Player Off"),
	One = DSCoreTypes::LedMasks::Middle UMETA(DisplayName = "Player One"),
	Two = DSCoreTypes::LedMasks::MiddleRight | DSCoreTypes::LedMasks::MiddleLeft UMETA(DisplayName = "Player Two"),
	Three = DSCoreTypes::LedMasks::Right | DSCoreTypes::LedMasks::Middle | DSCoreTypes::LedMasks::Left UMETA(DisplayName = "Player Three"),
	All = DSCoreTypes::LedMasks::Right | DSCoreTypes::LedMasks::MiddleRight | DSCoreTypes::LedMasks::MiddleLeft | DSCoreTypes::LedMasks::Left UMETA(DisplayName = "Player all led")
};

/**
 * @brief Enum defining brightness levels for LED effects on the DualSense controller.
 *
 * This enumeration specifies different brightness levels that can be assigned
 * to LED effects. It is designed to work with Blueprint environments.
 */
UENUM(BlueprintType)
enum class ELedBrightnessEnum : uint8
{
	Low = 0x03 UMETA(DisplayName = "Brightness Low"),
	Medium = 0x02 UMETA(DisplayName = "Brightness Medium"),
	High = 0x00 UMETA(DisplayName = "Brightness High")
};

/**
 * @brief Enum class representing audio feature states for a DualSense controller.
 *
 * This enum class is utilized to manage the audio-related features of a DualSense controller,
 * such as enabling or disabling audio functionality.
 *
 * Enum values:
 * - On: Indicates that the audio is enabled.
 * - Off: Indicates that the audio is disabled.
 */
UENUM(BlueprintType)
enum class EDualSenseAudioFeatureReport : uint8
{
	On UMETA(DisplayName = "Audio On"),
	Off UMETA(DisplayName = "Audio Off")
};

/**
 * @brief Enum class representing the feature report options for a DualSense device.
 *
 * This enum class defines the various modes that can be used for configuring the DualSense device's
 * haptic feedback and vibration functionalities.
 *
 * Enum values:
 * - Off: Disables all vibration and haptic feedback.
 * - DefaultRumble: Activates the default vibration mode.
 * - HapticSoftRumble: Enables audio-based vibration for a softer haptic experience.
 */
UENUM(BlueprintType)
enum class EDualSenseDeviceFeatureReport : uint8
{
	Off = 0,
	DefaultRumble = 0xFF UMETA(DisplayName = "Default"),
	HapticsRumble = 0x0C UMETA(DisplayName = "Audio based vibration")
};

/**
 * @brief Enum class defining the softness levels for DualSense controller triggers.
 *
 * This enum specifies different levels of trigger softness for DualSense controllers, which can
 * be used to adjust the tactile feedback of the triggers based on the desired gaming experience.
 *
 * Enum values:
 * - Off: Disables trigger softness entirely.
 * - VeryRigid: Represents a very rigid trigger feedback.
 * - Rigid: Represents rigid trigger feedback.
 * - Medium: Represents a medium level of softness for the trigger.
 * - Soft: Represents a soft trigger feedback.
 * - VerySoft: Represents a very soft trigger feedback.
 */
UENUM(BlueprintType)
enum class EDualSenseTriggerSoftnessLevel : uint8
{
	Off = 0,
	VeryRigid = 1 UMETA(DisplayName = "Very Rigid"),
	Rigid = 2 UMETA(DisplayName = "Rigid"),
	Medium = 4 UMETA(DisplayName = "Medium"),
	Soft = 6 UMETA(DisplayName = "Soft"),
	VerySoft = 8 UMETA(DisplayName = "Very Soft")
};

UENUM(BlueprintType)
enum class ETriggerFeedbackIntensity : uint8
{
	/** No force feedback applied */
	Disabled = 0x00 UMETA(DisplayName = "Disabled"),

	/** Low intensity force feedback */
	Low = 0x01 UMETA(DisplayName = "Low (25%)"),

	/** Medium intensity force feedback */
	Medium = 0x02 UMETA(DisplayName = "Medium (50%)"),

	/** High intensity force feedback */
	High = 0x03 UMETA(DisplayName = "High (100%)")
};

UENUM(BlueprintType)
enum class ETriggerForceIntensity : uint8
{
	/** No force feedback applied */
	Disabled = 0x00 UMETA(DisplayName = "Disabled"),

	/** Low intensity force feedback */
	Low = 0x01 UMETA(DisplayName = "Low (25%)"),

	/** Medium intensity force feedback */
	Medium = 0x02 UMETA(DisplayName = "Medium (50%)"),

	/** High intensity force feedback */
	High = 0x03 UMETA(DisplayName = "High (100%)")
};

UENUM(BlueprintType)
enum class ETriggerPosition : uint8
{
	/** No resistance at any position */
	Off = 0x00 UMETA(DisplayName = "Off (No Resistance)"),

	/** Start position - Beginning of trigger pull (0-25%) */
	Start = 0x82 UMETA(DisplayName = "Start (0-25%)"),

	/** Middle position - Mid trigger pull (25-50%) */
	Middle = 0x84 UMETA(DisplayName = "Middle (25-50%)"),

	/** Before End position - Near full pull (50-75%) */
	BeforeEnd = 0x88 UMETA(DisplayName = "Before End (50-75%)"),

	/** End position - Full trigger pull (75-100%) */
	End = 0x80 UMETA(DisplayName = "End (75-100%)")
};

UENUM(BlueprintType)
enum class ETriggerPositionMask : uint8
{
	Off = 0x00 UMETA(DisplayName = "Off (No Resistance)"),
	Start = 0x00 UMETA(DisplayName = "Start (0%)"),
	Early = 0x40 UMETA(DisplayName = "Early (25%)"),
	Middle = 0x80 UMETA(DisplayName = "Middle (50%)"),
	Late = 0xC0 UMETA(DisplayName = "Late (75%)"),
	End = 0xFF UMETA(DisplayName = "End (100%)"),
};

UENUM(BlueprintType)
enum class ETriggerForceMask : uint8
{
	Off = 0x00 UMETA(DisplayName = "Off (No Resistance)"),
	VeryLow = 0x20 UMETA(DisplayName = "Very Low (~12%)"),
	Low = 0x40 UMETA(DisplayName = "Low (25%)"),
	MediumLow = 0x60 UMETA(DisplayName = "Medium-Low (37%)"),
	Medium = 0x80 UMETA(DisplayName = "Medium (50%)"),
	MediumHigh = 0xA0 UMETA(DisplayName = "Medium-High (62%)"),
	High = 0xC0 UMETA(DisplayName = "High (75%)"),
	VeryHigh = 0xE0 UMETA(DisplayName = "Very High (87%)"),
	Max = 0xFF UMETA(DisplayName = "Max (100%)"),
};

UENUM(BlueprintType)
enum class EDualSenseTriggerAmplitude : uint8
{
	/** No amplitude - Effect disabled */
	None = 0x00 UMETA(DisplayName = "None (0%)"),

	/** Low amplitude - Subtle vibration effect */
	Low = 0x0A UMETA(DisplayName = "Low (~40%)"),

	/** Medium amplitude - Moderate vibration effect */
	Medium = 0x0C UMETA(DisplayName = "Medium (~50%)"),

	/** High amplitude - Strong vibration effect */
	High = 0x0F UMETA(DisplayName = "High (100%)")
};

UENUM(BlueprintType)
enum class EDualSenseSnapBack : uint8
{
	/** No SnapBack - Effect disabled */
	None = 0x00 UMETA(DisplayName = "None (0%)"),
	Low = 0x3A UMETA(DisplayName = "Low (~40%)"),
	Medium = 0x3E UMETA(DisplayName = "Medium (~50%)"),
	High = 0x3F UMETA(DisplayName = "High (100%)"),
};

UENUM(BlueprintType)
enum class EDualSenseWeaponTrigger : uint8
{
	/** No WeaponClick - Effect disabled */
	None = 0x00 UMETA(DisplayName = "None (0%)"),
	Low = 0x02 UMETA(DisplayName = "Low (~40%)"),
	Medium = 0x04 UMETA(DisplayName = "Medium (~50%)"),
	High = 0x0A UMETA(DisplayName = "High (~75%)"),
	Max = 0x0F UMETA(DisplayName = "Max (100%)"),
};

UENUM(BlueprintType)
enum class ETriggerEffectBehavior : uint8
{
	/** Effect applies only at specified position range */
	Localized = 0 UMETA(DisplayName = "Localized (Stop at Position)"),

	/** Effect continues until trigger is fully pressed */
	Sustained = 1 UMETA(DisplayName = "Sustained (Extend to End)")
};

UENUM(BlueprintType)
enum class EAutoGunStrength : uint8
{
	Light = 0 UMETA(DisplayName = "Light Recoil"),
	Medium = 1 UMETA(DisplayName = "Medium Recoil"),
	Heavy = 2 UMETA(DisplayName = "Heavy Recoil")
};

UENUM(BlueprintType)
enum class EGamepadHand : uint8
{
	Left,
	Right,
	AnyHand
};
