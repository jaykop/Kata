// Copyright (c) 2025 Rafael Valoto/Publisher. All rights reserved.
// Created for: WindowsDualsense_ds5w - Plugin to support DualSense controller on Windows.
// Planned Release Year: 2025

#pragma once

#include "API/Types/Enums/EDeviceCommons.h"
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SonyGamepadTriggerProxy.generated.h"

USTRUCT(BlueprintType)
struct FSliderPropertiesStruct
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slider",
	          meta = (ClampMin = "0.0", ClampMax = "40.0", UIMin = "0.0", UIMax = "40.0"))
	float Frequency = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slider",
	          meta = (ClampMin = "0.0", ClampMax = "20.0", UIMin = "0.0", UIMax = "20.0"))
	float Period = 6.0f;
};

USTRUCT(BlueprintType)
struct FSliderPropertyStruct
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slider",
	          meta = (ClampMin = "0.0", ClampMax = "40.0", UIMin = "0.0", UIMax = "40.0"))
	float Frequency = 5.0f;
};

/**
 *
 */
UCLASS()
class WINDOWSDUALSENSE_DS5W_API USonyGamepadTriggerProxy : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * @brief Configures the "GameCube" trigger effect on a DualSense controller.
	 *
	 * This function applies a special trigger effect designed to simulate the feedback characteristics
	 * associated with a GameCube-style response. It can be applied selectively to the left or right
	 * hand trigger on the specified DualSense controller.
	 *
	 * @param ControllerId The ID of the controller on which the effect is to be applied.
	 * @param Hand Specifies the hand (left or right) to which the trigger effect should be targeted.
	 */
	UFUNCTION(BlueprintCallable, Category = "SonyGamepadTrigger", meta = (DisplayName = "GameCube (0x02)"))
	static void GameCube(
	    int32 ControllerId,
	    EGamepadHand Hand);

	/**
	 * Applies a continuous resistance effect on the adaptive trigger of a DualSense controller.
	 *
	 * @param ControllerId The ID of the controller to apply the effect to.
	 * @param StartPosition The starting position of the resistance effect. Valid range determined by internal validation.
	 * @param Strength The intensity of the resistance effect. Valid range determined by internal validation.
	 * @param Hand The controller hand (e.g., left or right) to which the resistance effect applies.
	 */
	UFUNCTION(BlueprintCallable, Category = "SonyGamepadTrigger", meta = (DisplayName = "Resistance (0x01)"))
	static void Resistance(
	    int32 ControllerId,
	    ETriggerPositionMask StartPosition,
	    ETriggerForceMask Strength,
	    EGamepadHand Hand);

	/**
	 * @brief Configures the "Bow" trigger effect on a DualSense controller.
	 *
	 * This function applies a specific trigger feedback effect designed to emulate
	 * the response of a bow, targeting the specified start zone, snap-back behavior,
	 * and controller hand. It customizes the feedback on the trigger to align with a bow-like
	 * mechanic.
	 *
	 * @param ControllerId The unique identifier of the controller on which the effect is applied.
	 * @param StartZone Specifies the initial trigger position where the effect begins (e.g., start, middle).
	 * @param SnapBack Defines the snap-back behavior of the trigger after it is released.
	 * @param Hand Indicates the hand (left or right) where the trigger effect is applied.
	 */
	UFUNCTION(BlueprintCallable, Category = "SonyGamepadTrigger", meta = (DisplayName = "Bow (0x22)", ToolTip = "Configures the response of a bow, targeting the specified start zone, snap-back behavior"))
	static void Bow(
	    int32 ControllerId,
	    ETriggerPosition StartZone,
	    EDualSenseSnapBack SnapBack,
	    EGamepadHand Hand);

	/**
	 * @brief Configures the "Weapon" trigger effect on a DualSense controller.
	 *
	 * This function applies a weapon-specific feedback effect that emulates a responsive trigger pull characteristic
	 * commonly associated with weapon dynamics in games. The effect can be customized based on the starting trigger position,
	 * vibration amplitude, behavior, and specific weapon effect type. It can be applied to either the left or right trigger.
	 *
	 * @param ControllerId The unique identifier of the controller on which the effect is applied.
	 * @param StartZone Specifies the initial position on the trigger where the effect begins.
	 * @param Amplitude Determines the intensity of the vibration feedback.
	 * @param Behavior Defines the nature of the effect, such as localized or sustained.
	 * @param Trigger Specifies the weapon effect type to emulate (e.g., pistol, shotgun).
	 * @param Hand Indicates the hand (left or right) where the trigger effect is applied.
	 */
	UFUNCTION(BlueprintCallable, Category = "SonyGamepadTrigger", meta = (DisplayName = "Weapon (0x25)", ToolTip = "Configures the Weapon (0x25) trigger effect on a DualSense controller."))
	static void Weapon(int32 ControllerId, ETriggerPosition StartZone, EDualSenseTriggerAmplitude Amplitude,
	                   ETriggerEffectBehavior Behavior,
	                   EDualSenseWeaponTrigger Trigger, EGamepadHand Hand);

	/**
	 * @brief Configures the "Automatic Gun" trigger effect on a DualSense controller.
	 *
	 * This function applies a trigger effect simulating the feedback of an automatic gun
	 * on the specified DualSense controller. It allows customization of the starting trigger
	 * position, effect behavior, recoil intensity, frequency of the effect, and target hand.
	 *
	 * @param ControllerId The ID of the controller to apply the effect to.
	 * @param StartZone Specifies the initial position on the trigger where the effect begins.
	 * @param Behavior Defines the behavior of the effect (e.g., localized or sustained).
	 * @param Recoil Determines the strength of the recoil effect during feedback.
	 * @param Frequency The frequency of the effect, with a range of 0 to 40.
	 * @param Hand Specifies the hand (left or right) where the trigger effect should be applied.
	 */
	UFUNCTION(BlueprintCallable, Category = "SonyGamepadTrigger", meta = (DisplayName = "Automatic Gun (0x26)", ToolTip = "Configures the Automatic Gun (0x26) trigger effect on a DualSense controller."))
	static void AutomaticGun(
	    int32 ControllerId,
	    ETriggerPosition StartZone,
	    ETriggerEffectBehavior Behavior,
	    EAutoGunStrength Recoil,
	    FSliderPropertyStruct Frequency,
	    EGamepadHand Hand);

	/**
	 * @brief Configures the "Machine" trigger effect on a DualSense controller.
	 *
	 * This function applies a detailed trigger effect resembling a machine-like feedback pattern
	 * to the specified DualSense controller. The effect's characteristics, such as the start zone,
	 * behavior, force intensity, amplitude, period, and frequency, can be customized. The effect can
	 * be directed to either the left or right hand trigger.
	 *
	 * @param ControllerId The ID of the controller to apply the effect to.
	 * @param StartZone Specifies the starting position of the effect on the trigger.
	 * @param Behavior Defines the behavior of the effect (localized or sustained).
	 * @param ForceIntensity Simulates the strength of the force feedback effect.
	 * @param Amplitude Determines the vibration amplitude for the effect.
	 * @param Frequency_Period Defines the duration of each feedback cycle Frequency & Period (0-40, 0-20).
	 * @param Hand Identifies the hand (left or right) where the trigger effect should be applied.
	 */
	UFUNCTION(BlueprintCallable, Category = "SonyGamepadTrigger", meta = (DisplayName = "Machine (0x27)", ToolTip = "Configures the Machine (0x27) trigger effect on a DualSense controller."))
	static void Machine(
	    int32 ControllerId,
	    ETriggerPosition StartZone,
	    ETriggerEffectBehavior Behavior,
	    ETriggerForceIntensity ForceIntensity,
	    EDualSenseTriggerAmplitude Amplitude,
	    UPARAM(DisplayName = "Frequency & Period (0-40, 0-20)")
	        FSliderPropertiesStruct Frequency_Period,
	    EGamepadHand Hand);

	/**
	 * @brief Applies a custom trigger effect on a DualSense controller.
	 *
	 * This function sets a user-defined trigger effect by accepting a series of hexadecimal byte strings
	 * that define the desired behavior or feedback. The effect can be customized and applied to the left
	 * or right trigger on the specified controller.
	 *
	 * @param ControllerId The identifier of the target DualSense controller to apply the custom effect.
	 * @param HexBytes An array of hexadecimal byte strings that define the custom trigger effect.
	 * @param Hand Specifies whether the effect is to be applied to the left or right trigger.
	 *
	 */
	UFUNCTION(BlueprintCallable, Category = "SonyGamepadTrigger", meta = (DisplayName = "Custom Trigger (New)", ToolTip = "This function sets a user-defined trigger effect by accepting a series of hexadecimal byte strings that define the desired behavior or feedback. The effect can be customized and applied to the left or right trigger on the specified controller."))
	static void CustomTrigger(
	    int32 ControllerId,
	    const TArray<FString>& HexBytes,
	    EGamepadHand Hand);

	/**
	 * @brief Stops the active trigger effect on a specified hand of the given DualSense controller.
	 *
	 * This function disables any currently active trigger effect for the specified hand
	 * on the identified DualSense controller.
	 *
	 * @param ControllerId The ID of the controller where the trigger effect is to be stopped.
	 * @param Hand Specifies the hand (left or right) on which the trigger effect should be disabled.
	 */
	UFUNCTION(BlueprintCallable, Category = "SonyGamepadTrigger", meta = (DisplayName = "Stop Trigger (New)"))
	static void StopTrigger(int32 ControllerId, EGamepadHand Hand);
};
