// Copyright (c) 2025 Rafael Valoto/Publisher. All rights reserved.
// Created for: WindowsDualsense_ds5w - Plugin to support DualSense controller on Windows.
// Planned Release Year: 2025

#include "Implementations/Managers/HapticsRegistry.h"
#include "API/SonyGamepadProxyHelpers.h"
#include "Async/Async.h"
#include "AudioDevice.h"
#if PLATFORM_WINDOWS
#include "Implementations/Platforms/Windows/WindowsDeviceInfo.h"
#endif
#include "Misc/App.h"
#include "Runtime/Launch/Resources/Version.h"

TSharedPtr<FHapticsRegistry> FHapticsRegistry::Instance;

FHapticsRegistry::~FHapticsRegistry()
{
	RemoveAllListeners();
	if (Instance->GameThreadTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(Instance->GameThreadTickerHandle);
	}
}

bool FHapticsRegistry::HasListenerForDevice(int32 DeviceId) const
{
	return ControllerListeners.Contains(DeviceId);
}

TSharedPtr<FHapticsRegistry> FHapticsRegistry::Get()
{
	if (!Instance)
	{
		check(IsInGameThread());
		Instance = MakeShared<FHapticsRegistry>();

		Instance->GameThreadTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		    FTickerDelegate::CreateSP(Instance.Get(), &FHapticsRegistry::Tick));
	}
	return Instance;
}

void FHapticsRegistry::CreateListenerForDevice(int32 DeviceId, USoundSubmix* Submix)
{
	if (!Submix)
	{
		return;
	}

	using namespace SonyGamepadProxyHelpers;
	ISonyGamepad* Gamepad = GetGamepad(DeviceId);
	if (!Gamepad)
	{
		return;
	}

	if (Gamepad->GetConnectionType() == EDSDeviceConnection::Usb)
	{
		if (Gamepad->GetIGamepadHaptics())
		{
			FDeviceContext* Ctx = Gamepad->GetMutableDeviceContext();
			if (!Ctx->AudioContext || !Ctx->AudioContext->IsValid())
			{
				Ctx->AudioContext = std::make_shared<FAudioDeviceContext>();
#if PLATFORM_WINDOWS
				FWindowsDeviceInfo::InitializeAudioDevice(Ctx);
#endif
			}
		}
	}

	if (ControllerListeners.Contains(DeviceId))
	{
		UE_LOG(LogDualSense, Warning, TEXT("Controller %d already has a listener registered."), DeviceId);
		return;
	}

	bool IsWireless = Gamepad->GetConnectionType() == EDSDeviceConnection::Bluetooth;
	const TSharedPtr<FAudioHapticsListener> Listener = MakeShared<FAudioHapticsListener>(DeviceId, Submix, IsWireless);
	if (FAudioDeviceHandle AudioDevice = GEngine->GetActiveAudioDevice())
	{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION > 3
		AudioDevice->RegisterSubmixBufferListener(Listener.ToSharedRef(), *Submix);
#else
		AudioDevice->RegisterSubmixBufferListener(Listener.Get(), Submix);
#endif

		ControllerListeners.Add(DeviceId, Listener);
	}
}

void FHapticsRegistry::RemoveAllListeners()
{
	if (FAudioDeviceHandle AudioDevice = GEngine->GetActiveAudioDevice())
	{
		for (auto& Pair : ControllerListeners)
		{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION > 3
			AudioDevice->UnregisterSubmixBufferListener(Pair.Value.ToSharedRef(), *Pair.Value.ToSharedRef()->GetSubmix());
#else
			AudioDevice->UnregisterSubmixBufferListener(Pair.Value.Get());
#endif
		}
	}
	ControllerListeners.Empty();
}

bool FHapticsRegistry::Tick(float DeltaTime)
{
	for (auto& Pair : ControllerListeners)
	{
		if (Pair.Value.IsValid())
		{
			TSharedPtr<FAudioHapticsListener> Context = Pair.Value;
			if (IGamepadAudioHaptics* HapiticsInterface = SonyGamepadProxyHelpers::GetAudioHapticsInterface(Pair.Key))
			{
				AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [NewContext = MoveTemp(Context), NewHapiticsInterface = MoveTemp(HapiticsInterface)]() {
					NewContext->ConsumeHapticsQueue(NewHapiticsInterface);
				});
			}
		}
	}
	return true;
}

void FHapticsRegistry::RemoveListenerForDevice(int32 DeviceId)
{
	if (const TSharedPtr<FAudioHapticsListener>* ExistingListener = ControllerListeners.Find(DeviceId))
	{
		if (FAudioDeviceHandle AudioDevice = GEngine->GetActiveAudioDevice())
		{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION > 3
			AudioDevice->UnregisterSubmixBufferListener(ExistingListener->ToSharedRef(), *ExistingListener->ToSharedRef()->GetSubmix());
#else
			AudioDevice->UnregisterSubmixBufferListener(ExistingListener->Get());
#endif
		}
		ControllerListeners.Remove(DeviceId);
	}
}
