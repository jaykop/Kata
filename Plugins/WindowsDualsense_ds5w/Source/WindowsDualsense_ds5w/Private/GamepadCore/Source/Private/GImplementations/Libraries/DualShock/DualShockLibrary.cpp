// Copyright (c) 2025 Rafael Valoto. All Rights Reserved.
// Project: GamepadCore
// Description: Cross-platform library for DualSense and generic gamepad input support.
// Targets: Windows, Linux, macOS.

#include "GImplementations/Libraries/DualShock/DualShockLibrary.h"
#include "GCore/Interfaces/IPlatformHardwareInfo.h"
#include "GCore/Types/ECoreGamepad.h"
#include "GImplementations/Utils/GamepadInput.h"
#include "GImplementations/Utils/GamepadOutput.h"
#include "GImplementations/Utils/GamepadSensors.h"
#include "GImplementations/Utils/GamepadTouch.h"
#include <iomanip>

bool FDualShockLibrary::Initialize(const FDeviceContext& Context)
{
	SetDeviceContexts(Context);
	SetLightbarFlash({0, 0, 220, 0}, 0.0f, 0.0f);
	return true;
}

void FDualShockLibrary::UpdateOutput()
{
	FDeviceContext* Context = GetMutableDeviceContext();
	if (!Context->IsConnected)
	{
		return;
	}

	FGamepadOutput::OutputDualShock(Context);
}

void FDualShockLibrary::UpdateInput(float /*Delta*/)
{
	FDeviceContext* Context = GetMutableDeviceContext();
	IPlatformHardwareInfo::Get().Read(Context);
	// 읽기에 실패하면 InvalidateHandle이 HID 버퍼를 0으로 지운다. 원본은 그 버퍼를 그대로 파싱해, 스틱 원시값 0이
	// 정규화 후 (-1, +1)로 바뀌어 패드를 뽑은 순간 이동·시점 입력이 끝까지 밀린 채 고정되었다.
	// 파싱하지 않고 중립 상태를 게시해 게임 스레드가 입력을 놓게 한다.
	if (!Context->IsConnected)
	{
		*Context->GetBackBuffer() = FInputContext();
		Context->SwapInputBuffers();
		return;
	}

	FInputContext* InputToFill = Context->GetBackBuffer();

	using namespace FGamepadInput;
	const size_t Padding = Context->ConnectionType == EDSDeviceConnection::Bluetooth ? 3 : 1;
	if (Padding == 3)
	{
		DualShockRaw(&Context->BufferDS4[Padding], InputToFill);
		if (Context->bEnableGesture || Context->bEnableTouch)
		{
			using namespace FGamepadTouch;
			ProcessTouchDualShock(&Context->BufferDS4[Padding], InputToFill, Context->ConnectionType);
		}

		if (Context->bEnableAccelerometerAndGyroscope)
		{
			DSCoreTypes::DSVector3D GyroDeg;
			DSCoreTypes::DSVector3D AccelG;

			using namespace FGamepadSensors;
			ProcessMotionDualShock(&Context->BufferDS4[Padding], Context->Calibration, Context->ConnectionType, GyroDeg, AccelG);

			InputToFill->Gyroscope = GyroDeg;
			InputToFill->Accelerometer = AccelG;
		}
	}
	else
	{
		DualShockRaw(&Context->Buffer[Padding], InputToFill);
		if (Context->bEnableGesture || Context->bEnableTouch)
		{
			using namespace FGamepadTouch;
			ProcessTouchDualShock(&Context->Buffer[Padding], InputToFill, Context->ConnectionType);
		}

		if (Context->bEnableAccelerometerAndGyroscope)
		{
			DSCoreTypes::DSVector3D GyroDeg;
			DSCoreTypes::DSVector3D AccelG;

			using namespace FGamepadSensors;
			ProcessMotionDualShock(&Context->Buffer[Padding], Context->Calibration, Context->ConnectionType, GyroDeg, AccelG);

			InputToFill->Gyroscope = GyroDeg;
			InputToFill->Accelerometer = AccelG;
		}
	}

	Context->SwapInputBuffers();
}

void FDualShockLibrary::SetVibration(std::uint8_t LeftRumble, std::uint8_t RightRumble)
{
	FDeviceContext* Context = GetMutableDeviceContext();
	if (!Context)
	{
		return;
	}

	FOutputContext* HidOutput = &Context->Output;
	if (HidOutput->Rumbles.Left != LeftRumble ||
	    HidOutput->Rumbles.Right != RightRumble)
	{
		HidOutput->Rumbles = {LeftRumble, RightRumble};
	}
}

void FDualShockLibrary::SetLightbarFlash(DSCoreTypes::FDSColor Color, float BrithnessTime, float ToggleTime)
{
	FDeviceContext* Context = GetMutableDeviceContext();
	FOutputContext* HidOutput = &Context->Output;
	HidOutput->Lightbar.R = Color.R;
	HidOutput->Lightbar.G = Color.G;
	HidOutput->Lightbar.B = Color.B;

	HidOutput->FlashLigthbar.Bright_Time = static_cast<std::uint8_t>((BrithnessTime / 2.5f) * 255);
	HidOutput->FlashLigthbar.Toggle_Time = static_cast<std::uint8_t>((ToggleTime / 2.5f) * 255);
}

void FDualShockLibrary::ResetLights()
{
	SetLightbarFlash({0, 0, 255, 0}, 0.0f, 0.0f);
}
