// Copyright (c) 2025 Rafael Valoto/Publisher. All rights reserved.
// Created for: WindowsDualsense_ds5w - Plugin to support DualSense controller on Windows.
// Planned Release Year: 2025

#include "API/SonyGamepadHapticsProxy.h"
#include "API/SonyGamepadProxyHelpers.h"
#include "Implementations/Managers/HapticsRegistry.h"

using namespace SonyGamepadProxyHelpers;
void USonyGamepadHapticsProxy::RegisterSubmixForDevice(int32 ControllerId, USoundSubmix* Submix)
{
	if (GetAudioHapticsInterface(ControllerId))
	{
		FHapticsRegistry::Get()->CreateListenerForDevice(ControllerId, Submix);
	}
}

void USonyGamepadHapticsProxy::UnregisterSubmixForDevice(int32 ControllerId)
{
	if (GetAudioHapticsInterface(ControllerId))
	{
		FHapticsRegistry::Get()->RemoveListenerForDevice(ControllerId);
	}
}
