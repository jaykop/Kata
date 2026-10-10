// Copyright (c) 2025 Rafael Valoto/Publisher. All rights reserved.
// Created for: WindowsDualsense_ds5w - Plugin to support DualSense controller on Windows.
// Planned Release Year: 2025

#include "API/SonyGamepadTriggerProxy.h"
#include "API/SonyGamepadProxyHelpers.h"
#include "API/Types/Enums/EDeviceCommons.h"

using namespace SonyGamepadProxyHelpers;

void USonyGamepadTriggerProxy::GameCube(int32 ControllerId, EGamepadHand Hand)
{
	if (IGamepadTrigger* Gamepad = GetTriggerInterface(ControllerId))
	{
		Gamepad->SetGameCube(static_cast<EDSGamepadHand>(Hand));
	}
}
void USonyGamepadTriggerProxy::Resistance(int32 ControllerId, ETriggerPositionMask StartPosition,
                                          ETriggerForceMask Strength, EGamepadHand Hand)
{
	if (IGamepadTrigger* Gamepad = GetTriggerInterface(ControllerId))
	{
		Gamepad->SetResistance(static_cast<uint8>(StartPosition), static_cast<uint8>(Strength), static_cast<EDSGamepadHand>(Hand));
	}
}
void USonyGamepadTriggerProxy::Bow(int32 ControllerId, ETriggerPosition StartZone, EDualSenseSnapBack SnapBack,
                                   EGamepadHand Hand)
{
	if (IGamepadTrigger* Gamepad = GetTriggerInterface(ControllerId))
	{
		Gamepad->SetBow22(static_cast<uint8>(StartZone), static_cast<uint8>(SnapBack), static_cast<EDSGamepadHand>(Hand));
	}
}
void USonyGamepadTriggerProxy::Weapon(int32 ControllerId, ETriggerPosition StartZone,
                                      EDualSenseTriggerAmplitude Amplitude, ETriggerEffectBehavior Behavior,
                                      EDualSenseWeaponTrigger Trigger, EGamepadHand Hand)
{
	if (IGamepadTrigger* Gamepad = GetTriggerInterface(ControllerId))
	{
		Gamepad->SetWeapon25(static_cast<uint8>(StartZone), static_cast<uint8>(Amplitude), static_cast<uint8>(Behavior),
		                     static_cast<uint8>(Trigger), static_cast<EDSGamepadHand>(Hand));
	}
}
void USonyGamepadTriggerProxy::AutomaticGun(int32 ControllerId, ETriggerPosition StartZone,
                                            ETriggerEffectBehavior Behavior, EAutoGunStrength Recoil, FSliderPropertyStruct Frequency, EGamepadHand Hand)
{
	if (IGamepadTrigger* Gamepad = GetTriggerInterface(ControllerId))
	{
		Gamepad->SetMachineGun26(static_cast<uint8>(StartZone), static_cast<uint8>(Behavior), static_cast<uint8>(Recoil), Frequency.Frequency, static_cast<EDSGamepadHand>(Hand));
	}
}
void USonyGamepadTriggerProxy::Machine(int32 ControllerId, ETriggerPosition StartZone, ETriggerEffectBehavior Behavior,
                                       ETriggerForceIntensity ForceIntensity, EDualSenseTriggerAmplitude Amplitude,
                                       FSliderPropertiesStruct Frequency_Period,
                                       EGamepadHand Hand)
{
	if (IGamepadTrigger* Gamepad = GetTriggerInterface(ControllerId))
	{
		Gamepad->SetMachine27(static_cast<uint8>(StartZone), static_cast<uint8>(Behavior), static_cast<uint8>(ForceIntensity), static_cast<uint8>(Amplitude),
		                      Frequency_Period.Period, Frequency_Period.Frequency, static_cast<EDSGamepadHand>(Hand));
	}
}
void USonyGamepadTriggerProxy::CustomTrigger(int32 ControllerId, const TArray<FString>& HexBytes, EGamepadHand Hand)
{
	if (HexBytes.Num() > 10)
	{
		return;
	}

	if (IGamepadTrigger* Gamepad = GetTriggerInterface(ControllerId))
	{
		Gamepad->SetCustomTrigger(static_cast<EDSGamepadHand>(Hand), ConvertHexStringsToBytes(HexBytes));
	}
}
void USonyGamepadTriggerProxy::StopTrigger(int32 ControllerId, EGamepadHand Hand)
{
	if (IGamepadTrigger* Gamepad = GetTriggerInterface(ControllerId))
	{
		Gamepad->StopTrigger(static_cast<EDSGamepadHand>(Hand));
	}
}
