// Copyright (c) 2025 Rafael Valoto/Publisher. All rights reserved.
// Created for: WindowsDualsense_ds5w - Plugin to support DualSense controller on Windows.
// Planned Release Year: 2025

#pragma once

#include "GCore/Interfaces/ISonyGamepad.h"
#include "GenericPlatform/GenericApplicationMessageHandler.h"
#include "Helpers/DualSenseLog.h"
#include "Implementations/Adapters/DeviceRegistry.h"
#include "Types/Enums/EDeviceCommons.h"
#include "Types/Enums/EDeviceConnection.h"

// Forward declarations
class IGamepadTrigger;
class IGamepadAudioHaptics;

static_assert(static_cast<uint8>(EDeviceType::DualSense) == static_cast<uint8>(EDSDeviceType::DualSense), "Enum mismatch: DualSense value differs between Core and API!");
static_assert(static_cast<uint8>(EDeviceType::DualShock4) == static_cast<uint8>(EDSDeviceType::DualShock4), "Enum mismatch: DualShock4 value differs between Core and API!");
static_assert(static_cast<uint8>(EDeviceType::DualSenseEdge) == static_cast<uint8>(EDSDeviceType::DualSenseEdge), "Enum mismatch: DualSenseEdge value differs between Core and API!");

static_assert(static_cast<uint8>(EDeviceConnection::Usb) == static_cast<uint8>(EDSDeviceConnection::Usb), "Enum mismatch: DeviceConnection USB value differs between Core and API!");
static_assert(static_cast<uint8>(EDeviceConnection::Bluetooth) == static_cast<uint8>(EDSDeviceConnection::Bluetooth), "Enum mismatch: DeviceConnection Bluetooth value differs between Core and API!");
static_assert(static_cast<uint8>(EDeviceConnection::Unrecognized) == static_cast<uint8>(EDSDeviceConnection::Unrecognized), "Enum mismatch: DeviceConnection Unrecognized value differs between Core and API!");

static_assert(static_cast<uint8>(EGamepadHand::Left) == static_cast<uint8>(EDSGamepadHand::Left), "Enum mismatch: GamepadHand Left value differs between Core and API!");
static_assert(static_cast<uint8>(EGamepadHand::Right) == static_cast<uint8>(EDSGamepadHand::Right), "Enum mismatch: GamepadHand Right value differs between Core and API!");
static_assert(static_cast<uint8>(EGamepadHand::AnyHand) == static_cast<uint8>(EDSGamepadHand::AnyHand), "Enum mismatch: GamepadHand AnyHand value differs between Core and API!");

static_assert(static_cast<uint8>(EDualSenseAudioFeatureReport::On) == static_cast<uint8>(EDSAudioMode::On), "Enum mismatch: AudioFeature On value differs between Core and API!");
static_assert(static_cast<uint8>(EDualSenseAudioFeatureReport::Off) == static_cast<uint8>(EDSAudioMode::Off), "Enum mismatch: AudioFeature Off value differs between Core and API!");

static_assert(static_cast<uint8>(ELedPlayerEnum::Off) == static_cast<uint8>(EDSPlayer::Off), "Enum mismatch: LedPlayer Off value differs between Core and API!");
static_assert(static_cast<uint8>(ELedPlayerEnum::One) == static_cast<uint8>(EDSPlayer::One), "Enum mismatch: LedPlayer One value differs between Core and API!");
static_assert(static_cast<uint8>(ELedPlayerEnum::Two) == static_cast<uint8>(EDSPlayer::Two), "Enum mismatch: LedPlayer Two value differs between Core and API!");
static_assert(static_cast<uint8>(ELedPlayerEnum::Three) == static_cast<uint8>(EDSPlayer::Three), "Enum mismatch: LedPlayer Three value differs between Core and API!");
static_assert(static_cast<uint8>(ELedPlayerEnum::All) == static_cast<uint8>(EDSPlayer::All), "Enum mismatch: LedPlayer All value differs between Core and API!");

static_assert(static_cast<uint8>(ELedMicEnum::MicOn) == static_cast<uint8>(EDSMic::MicOn), "Enum mismatch: LedMic MicOn value differs between Core and API!");
static_assert(static_cast<uint8>(ELedMicEnum::MicOff) == static_cast<uint8>(EDSMic::MicOff), "Enum mismatch: LedMic MicOff value differs between Core and API!");
static_assert(static_cast<uint8>(ELedMicEnum::Pulse) == static_cast<uint8>(EDSMic::Pulse), "Enum mismatch: LedMic Pulse value differs between Core and API!");

/**
 * @namespace SonyGamepadProxyHelpers
 * @brief Utility functions to reduce boilerplate code in Blueprint proxy classes.
 *
 * Thread Safety: All functions must be called from the Game Thread
 */
namespace SonyGamepadProxyHelpers
{
	/**
	 * @brief Finds the ISonyGamepad instance for a given controller ID.
	 *
	 * @param ControllerId The platform user ID (0-3 typically)
	 * @return ISonyGamepad pointer or nullptr if not found
	 */
	inline ISonyGamepad* GetGamepad(int32 ControllerId)
	{
		check(IsInGameThread());

		TArray<FInputDeviceId> Devices;
		IPlatformInputDeviceMapper::Get().GetAllInputDevicesForUser(
		    FPlatformUserId::CreateFromInternalId(ControllerId),
		    Devices);

		FDeviceRegistry* Registry = FDeviceRegistry::Get();
		for (const FInputDeviceId& DeviceId : Devices)
		{
			if (ISonyGamepad* Gamepad = Registry->GetLibraryInstance(DeviceId))
			{
				return Gamepad;
			}
		}
		return nullptr;
	}

	/**
	 * @brief Gets the trigger interface for a controller.
	 *
	 * @param ControllerId The controller ID
	 * @return IGamepadTrigger pointer or nullptr
	 */
	inline IGamepadTrigger* GetTriggerInterface(int32 ControllerId)
	{
		ISonyGamepad* Gamepad = GetGamepad(ControllerId);
		if (!Gamepad)
		{
			return nullptr;
		}

		IGamepadTrigger* TriggerInterface = Gamepad->GetIGamepadTrigger();
		if (!TriggerInterface)
		{
			return nullptr;
		}

		return TriggerInterface;
	}

	/**
	 * @brief Gets the audio haptics interface for a controller.
	 *
	 * @param ControllerId The controller ID
	 * @return IGamepadAudioHaptics pointer or nullptr
	 */
	inline IGamepadAudioHaptics* GetAudioHapticsInterface(int32 ControllerId)
	{
		ISonyGamepad* Gamepad = GetGamepad(ControllerId);
		if (!Gamepad)
		{
			return nullptr;
		}

		IGamepadAudioHaptics* AudioInterface = Gamepad->GetIGamepadHaptics();
		if (!AudioInterface)
		{
			return nullptr;
		}

		return AudioInterface;
	}

	/**
	 * @brief Converts ControllerId to FInputDeviceId (for low-level operations).
	 *
	 * @param ControllerId The controller ID
	 * @return Valid FInputDeviceId or invalid ID if not found
	 */
	inline FInputDeviceId GetDeviceId(int32 ControllerId)
	{
		check(IsInGameThread());

		TArray<FInputDeviceId> Devices;
		IPlatformInputDeviceMapper::Get().GetAllInputDevicesForUser(
		    FPlatformUserId::CreateFromInternalId(ControllerId),
		    Devices);

		FDeviceRegistry* Registry = FDeviceRegistry::Get();
		for (const FInputDeviceId& DeviceId : Devices)
		{
			if (Registry->GetLibraryInstance(DeviceId))
			{
				return DeviceId;
			}
		}

		return FInputDeviceId::CreateFromInternalId(INDEX_NONE);
	}

	inline std::vector<std::uint8_t> ConvertHexStringsToBytes(const TArray<FString>& UnrealArray)
	{
		std::vector<uint8_t> CoreBytes;
		CoreBytes.reserve(UnrealArray.Num());
		for (const FString& HexStr : UnrealArray)
		{
			CoreBytes.push_back(static_cast<uint8_t>(FParse::HexNumber(*HexStr)));
		}
		return CoreBytes;
	}

} // namespace SonyGamepadProxyHelpers
