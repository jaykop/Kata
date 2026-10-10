// Copyright (c) 2025 Rafael Valoto/Publisher. All rights reserved.
// Created for: WindowsDualsense_ds5w - Plugin to support DualSense controller on Windows.
// Planned Release Year: 2025

#include "API/SonyGamepadLightsProxy.h"
#include "API/SonyGamepadProxyHelpers.h"
#include "GCore/Types/DSCoreTypes.h"

using namespace DSCoreTypes;
using namespace SonyGamepadProxyHelpers;

void USonyGamepadLightsProxy::Lightbar(int32 ControllerId, FColor Color)
{
	if (ISonyGamepad* Gamepad = GetGamepad(ControllerId))
	{
		FDSColor CastColor = {Color.R, Color.G, Color.B, Color.A};
		Gamepad->SetLightbar(CastColor);
	}
}

void USonyGamepadLightsProxy::LightbarFlash(int32 ControllerId, FColor Color, float BrightnessTime, float ToggleTime)
{
	if (ISonyGamepad* Gamepad = GetGamepad(ControllerId))
	{
		FDSColor CastColor = {Color.R, Color.G, Color.B, Color.A};
		Gamepad->SetLightbarFlash(CastColor, BrightnessTime, ToggleTime);
	}
}

void USonyGamepadLightsProxy::PlayerLed(int32 ControllerId, ELedPlayerEnum Value, ELedBrightnessEnum Brightness)
{
	if (ISonyGamepad* Gamepad = GetGamepad(ControllerId))
	{
		Gamepad->SetPlayerLed(static_cast<EDSPlayer>(Value), static_cast<std::uint8_t>(Brightness));
	}
}
