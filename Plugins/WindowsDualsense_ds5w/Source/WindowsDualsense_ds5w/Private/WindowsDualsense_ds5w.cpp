// Copyright (c) 2025 Rafael Valoto/Publisher. All rights reserved.
// Created for: WindowsDualsense_ds5w - Plugin to support DualSense controller on Windows.
// Planned Release Year: 2025

#include "WindowsDualsense_ds5w/Public/WindowsDualsense_ds5w.h"
#include "API/SonyGamepadProxyHelpers.h"
#include "GCore/Interfaces/IPlatformHardwareInfo.h"
#include "Helpers/DualSenseLog.h"
#include "Implementations/Adapters/DeviceRegistry.h"
#include "Implementations/Platforms/Windows/WindowsHardwarePolicy.h"

#include "DeviceManager.h"
#include "InputCoreTypes.h"
#include "Misc/Paths.h"

#define LOCTEXT_NAMESPACE "FWindowsDualsense_ds5wModule"

void FWindowsDualsense_ds5wModule::StartupModule()
{
	IModularFeatures::Get().RegisterModularFeature(GetModularFeatureName(), this);
	RegisterCustomKeys();

#if PLATFORM_WINDOWS
	// Initialize PlatformHardware, (e.g., FLinuxHardware FWindowsHardware FMacHardware, FSonyHardware)
	std::unique_ptr<IPlatformHardwareInfo> WindowsInstance = std::make_unique<FWindowsPlatform::FWindowsHardware>();
	IPlatformHardwareInfo::SetInstance(std::move(WindowsInstance));

	// Initialize FDeviceRegistry
	FDeviceRegistry::Initialize();
#endif
}

void FWindowsDualsense_ds5wModule::ShutdownModule()
{
	// 입력 리더 스레드를 DLL 언로드 전에 멈춘다.
	FDeviceRegistry::Shutdown();
}

FWindowsDualsense_ds5wModule::FCustomInputDeviceFactory FWindowsDualsense_ds5wModule::CustomInputDeviceFactory = nullptr;

TSharedPtr<IInputDevice> FWindowsDualsense_ds5wModule::CreateInputDevice(
    const TSharedRef<FGenericApplicationMessageHandler>& InCustomMessageHandler)
{
	if (CustomInputDeviceFactory)
	{
		return CustomInputDeviceFactory(InCustomMessageHandler);
	}

	return MakeShareable(new DeviceManager(InCustomMessageHandler));
}

void FWindowsDualsense_ds5wModule::SetCustomInputDeviceFactory(FCustomInputDeviceFactory Factory)
{
	CustomInputDeviceFactory = Factory;
}

void FWindowsDualsense_ds5wModule::RegisterCustomKeys()
{
	// 표준 게임패드 키와 겹치는 PS_PushLeftStick, PS_PushRightStick, PS_Menu, PS_Share는 등록하지 않는다.
	// DeviceManager도 이 키들을 보내지 않는다.
	const FKey Mic("PS_Mic");
	const FKey TouchButtom("PS_TouchButtom");
	const FKey PlayStationButton("PS_Button");
	const FKey PS_FunctionL("PS_FunctionL");
	const FKey PS_FunctionR("PS_FunctionR");
	const FKey PS_PaddleL("PS_PaddleL");
	const FKey PS_PaddleR("PS_PaddleR");

	EKeys::AddKey(FKeyDetails(
	    PS_FunctionL,
	    FText::FromString("PlayStation Left Function Button"),
	    FKeyDetails::GamepadKey));

	EKeys::AddKey(FKeyDetails(
	    PS_FunctionR,
	    FText::FromString("PlayStation Right Function Button"),
	    FKeyDetails::GamepadKey));

	EKeys::AddKey(FKeyDetails(
	    PS_PaddleL,
	    FText::FromString("PlayStation Left Paddle"),
	    FKeyDetails::GamepadKey));

	EKeys::AddKey(FKeyDetails(
	    PS_PaddleR,
	    FText::FromString("PlayStation Right Paddle"),
	    FKeyDetails::GamepadKey));

	EKeys::AddKey(FKeyDetails(
	    PlayStationButton,
	    FText::FromString("PlayStation Button"),
	    FKeyDetails::GamepadKey));

	EKeys::AddKey(FKeyDetails(
	    Mic,
	    FText::FromString("PlayStation Mic"),
	    FKeyDetails::GamepadKey));

	EKeys::AddKey(FKeyDetails(
	    TouchButtom,
	    FText::FromString("PlayStation Touchpad Button"),
	    FKeyDetails::GamepadKey));
}

IMPLEMENT_MODULE(FWindowsDualsense_ds5wModule, WindowsDualsense_ds5w)
