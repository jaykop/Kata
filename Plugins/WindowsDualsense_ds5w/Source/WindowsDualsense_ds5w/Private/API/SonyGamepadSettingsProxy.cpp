// Copyright (c) 2025 Rafael Valoto/Publisher. All rights reserved.
// Created for: WindowsDualsense_ds5w - Plugin to support DualSense controller on Windows.
// Planned Release Year: 2025

#include "API/SonyGamepadSettingsProxy.h"
#include "API/SonyGamepadProxyHelpers.h"

using namespace SonyGamepadProxyHelpers;
void USonyGamepadSettingsProxy::DualSenseSettings(int32 ControllerId, FDualSenseFeatureReport Value)
{
	FDualSenseLibrary* Gamepad = static_cast<FDualSenseLibrary*>(GetTriggerInterface(ControllerId));
	if (!Gamepad)
	{
		return;
	}

	Gamepad->DualSenseSettings(
	    static_cast<std::uint8_t>(Value.MicStatus),
	    static_cast<std::uint8_t>(Value.AudioHeadset),
	    static_cast<std::uint8_t>(Value.AudioSpeaker),
	    static_cast<std::uint8_t>(Value.MicVolume),
	    static_cast<std::uint8_t>(Value.AudioVolume),
	    static_cast<std::uint8_t>(Value.VibrationMode),
	    static_cast<std::uint8_t>(Value.SoftRumbleReduce),
	    static_cast<std::uint8_t>(Value.TriggerSoftnessLevel));
}
void USonyGamepadSettingsProxy::MadgwickBeta(float Value)
{
	PluginSettings::MadgwickBeta = Value;
}
void USonyGamepadSettingsProxy::PollInterval(float Value)
{
	PluginSettings::PollInterval = Value;
}
