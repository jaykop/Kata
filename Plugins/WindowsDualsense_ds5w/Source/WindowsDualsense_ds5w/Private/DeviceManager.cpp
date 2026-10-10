// Copyright (c) 2025 Rafael Valoto/Publisher. All rights reserved.
// Created for: WindowsDualsense_ds5w - Plugin to support DualSense controller on Windows.
// Planned Release Year: 2025

#include "DeviceManager.h"
#include "API/SonyGamepadProxyHelpers.h"
#include "API/SonyGamepadSettingsProxy.h"
#include "Async/Async.h"
#include "Async/TaskGraphInterfaces.h"
#include "GCore/Types/DSCoreTypes.h"
#include "GCore/Types/ECoreGamepad.h"
#include "GenericPlatform/IInputInterface.h"
#include "IInputDevice.h"
#include "InputCoreTypes.h"
#include "Misc/CoreDelegates.h"
#include "Runtime/Launch/Resources/Version.h"

using namespace DSCoreTypes;
using namespace SonyGamepadProxyHelpers;

DeviceManager::DeviceManager(const TSharedRef<FGenericApplicationMessageHandler>& InMessageHandler)
    : MessageHandler(InMessageHandler)
{
	FCoreDelegates::OnUserLoginChangedEvent.AddRaw(this, &DeviceManager::OnUserLoginChangedEvent);
	ConnectionChangeHandle = IPlatformInputDeviceMapper::Get().GetOnInputDeviceConnectionChange().AddRaw(
	    this, &DeviceManager::HandleInputDeviceConnectionChange);
}

DeviceManager::~DeviceManager()
{
	FCoreDelegates::OnUserLoginChangedEvent.RemoveAll(this);
	IPlatformInputDeviceMapper::Get().GetOnInputDeviceConnectionChange().Remove(ConnectionChangeHandle);
	FilterSensors.Empty();
}

void DeviceManager::Tick(float DeltaTime)
{
	FDeviceRegistry* Registry = FDeviceRegistry::Get();
	if (Registry)
	{
		Registry->DiscoverDevices(DeltaTime);
	}

	// HID 읽기는 장치별 FSonyGamepadReader 스레드가 맡고, 여기서는 매 Tick 최신 입력 상태로 이벤트를 보낸다.
	// 원본은 PollInterval마다 AsyncTask로 읽고 누산기를 0으로 리셋했다. 그래서 고주사율에서 폴링 주기가 프레임 단위로
	// 떨어졌고, 백그라운드 작업이 원시 포인터로 라이브러리를 쓰는 동안 게임 스레드가 같은 상태를 읽었다.
	SendControllerEvents(DeltaTime);
}

void DeviceManager::SendControllerEvents(float DeltaTime)
{
	TArray<FInputDeviceId> OutInputDevices;
	OutInputDevices.Reset();
	IPlatformInputDeviceMapper::Get().GetAllConnectedInputDevices(OutInputDevices);
	for (const FInputDeviceId& Device : OutInputDevices)
	{
		// GetLibraryInstance는 정적 함수다. 모듈 종료 뒤 Get()이 nullptr을 돌려줄 수 있어 인스턴스를 거치지 않는다.
		if (ISonyGamepad* Gamepad = FDeviceRegistry::GetLibraryInstance(Device); Gamepad)
		{
			const FPlatformUserId UserId = IPlatformInputDeviceMapper::Get().GetUserForInputDevice(Device);
			if (const int32 ControllerId = FPlatformMisc::GetUserIndexForPlatformUser(UserId); ControllerId == -1)
			{
				continue;
			}

			// 원본은 여기서 FInputDeviceScope로 장치 종류를 알렸다. UE 5.8에서 deprecated이고 효과가 없어,
			// 연결 시 FDeviceRegistryPolicy::RegisterDeviceDescriptor로 등록한다.
			if (FDeviceContext* Context = Gamepad->GetMutableDeviceContext())
			{
				// 리더 스레드가 입력 버퍼를 바꾸는 중에도 안전하도록 잠금 아래에서 복사한 상태를 쓴다.
				FInputContext FrameInput;
				Context->CopyInputState(FrameInput);
				CheckEvents(Context, FrameInput, UserId, Device, DeltaTime);
			}
		}
	}
}

void DeviceManager::SensorsImpl(FDeviceContext* Context, FInputContext& FrameInput, const FPlatformUserId UserId, const FInputDeviceId InputDeviceId, float DeltaTime) const
{
	if (Context->bEnableAccelerometerAndGyroscope)
	{
		float RawGyroX = FrameInput.Gyroscope.X;
		float RawGyroY = FrameInput.Gyroscope.Y;
		float RawGyroZ = FrameInput.Gyroscope.Z;
		float RawAcclX = FrameInput.Accelerometer.X;
		float RawAcclY = FrameInput.Accelerometer.Y;
		float RawAcclZ = FrameInput.Accelerometer.Z;

		constexpr float DEG_TO_RAD = 3.14159265358979323846f / 180.0f;
		float G_Roll = (RawGyroZ * DEG_TO_RAD);
		float G_Pitch = (RawGyroX * DEG_TO_RAD);
		float G_Yaw = -(RawGyroY * DEG_TO_RAD);
		float A_Roll = RawAcclZ;
		float A_Pitch = RawAcclX;
		float A_Yaw = -RawAcclY;

		if (!FilterSensors.Contains(InputDeviceId))
		{
			TSharedPtr<FMadgwickAhrs> NewSensor = MakeShared<FMadgwickAhrs>(PluginSettings::MadgwickBeta);
			FilterSensors.Add(InputDeviceId, NewSensor);
		}

		if (Context->bIsResetGyroscope)
		{
			FilterSensors[InputDeviceId]->Reset();
			Context->bIsResetGyroscope = false;
		}

		FilterSensors[InputDeviceId]->UpdateImu(G_Roll, G_Pitch, G_Yaw, A_Roll, A_Pitch, A_Yaw, DeltaTime);

		float qw, qx, qy, qz;
		FilterSensors[InputDeviceId]->GetQuaternion(qw, qx, qy, qz);
		const FQuat RawSensorQuat = FQuat(qx, qy, qz, qw);
		FRotator TiltRotator = RawSensorQuat.Rotator();
		FVector UnrealGyro(G_Roll, G_Pitch, G_Yaw);
		FVector UnrealAccel(A_Roll, A_Pitch, A_Yaw);
		FVector UnrealTilt = FVector(TiltRotator.Roll, TiltRotator.Pitch, TiltRotator.Yaw);

		MessageHandler.Get().OnMotionDetected(UnrealTilt, UnrealGyro, RawSensorQuat.GetUpVector(), UnrealAccel, UserId, InputDeviceId);
	}
}

void DeviceManager::CheckEvents(FDeviceContext* Context, FInputContext& FrameInput, const FPlatformUserId UserId, const FInputDeviceId InputDeviceId, float DeltaTime) const
{
	CheckButtonInput(Context, UserId, InputDeviceId, FGamepadKeyNames::LeftStickLeft, FrameInput.bLeftAnalogLeft);
	CheckButtonInput(Context, UserId, InputDeviceId, FGamepadKeyNames::LeftStickRight, FrameInput.bLeftAnalogRight);
	CheckButtonInput(Context, UserId, InputDeviceId, FGamepadKeyNames::LeftStickDown, FrameInput.bLeftAnalogDown);
	CheckButtonInput(Context, UserId, InputDeviceId, FGamepadKeyNames::LeftStickUp, FrameInput.bLeftAnalogUp);

	CheckButtonInput(Context, UserId, InputDeviceId, FGamepadKeyNames::RightStickLeft, FrameInput.bRightAnalogLeft);
	CheckButtonInput(Context, UserId, InputDeviceId, FGamepadKeyNames::RightStickRight, FrameInput.bRightAnalogRight);
	CheckButtonInput(Context, UserId, InputDeviceId, FGamepadKeyNames::RightStickDown, FrameInput.bRightAnalogDown);
	CheckButtonInput(Context, UserId, InputDeviceId, FGamepadKeyNames::RightStickUp, FrameInput.bRightAnalogUp);

	MessageHandler.Get().OnControllerAnalog(FGamepadKeyNames::LeftAnalogX, UserId, InputDeviceId, FrameInput.LeftAnalog.X);
	MessageHandler.Get().OnControllerAnalog(FGamepadKeyNames::LeftAnalogY, UserId, InputDeviceId, FrameInput.LeftAnalog.Y);
	MessageHandler.Get().OnControllerAnalog(FGamepadKeyNames::RightAnalogX, UserId, InputDeviceId, FrameInput.RightAnalog.X);
	MessageHandler.Get().OnControllerAnalog(FGamepadKeyNames::RightAnalogY, UserId, InputDeviceId, FrameInput.RightAnalog.Y);

	MessageHandler.Get().OnControllerAnalog(FGamepadKeyNames::LeftTriggerAnalog, UserId, InputDeviceId, FrameInput.LeftTriggerAnalog);
	MessageHandler.Get().OnControllerAnalog(FGamepadKeyNames::RightTriggerAnalog, UserId, InputDeviceId, FrameInput.RightTriggerAnalog);

	CheckButtonInput(Context, UserId, InputDeviceId, FGamepadKeyNames::FaceButtonBottom, FrameInput.bCross);
	CheckButtonInput(Context, UserId, InputDeviceId, FGamepadKeyNames::FaceButtonLeft, FrameInput.bSquare);
	CheckButtonInput(Context, UserId, InputDeviceId, FGamepadKeyNames::FaceButtonRight, FrameInput.bCircle);
	CheckButtonInput(Context, UserId, InputDeviceId, FGamepadKeyNames::FaceButtonTop, FrameInput.bTriangle);

	CheckButtonInput(Context, UserId, InputDeviceId, FGamepadKeyNames::DPadUp, FrameInput.bDpadUp);
	CheckButtonInput(Context, UserId, InputDeviceId, FGamepadKeyNames::DPadDown, FrameInput.bDpadDown);
	CheckButtonInput(Context, UserId, InputDeviceId, FGamepadKeyNames::DPadLeft, FrameInput.bDpadLeft);
	CheckButtonInput(Context, UserId, InputDeviceId, FGamepadKeyNames::DPadRight, FrameInput.bDpadRight);

	CheckButtonInput(Context, UserId, InputDeviceId, FGamepadKeyNames::LeftShoulder, FrameInput.bLeftShoulder);
	CheckButtonInput(Context, UserId, InputDeviceId, FGamepadKeyNames::RightShoulder, FrameInput.bRightShoulder);

	// mapped urenal native gamepad Start and Select
	CheckButtonInput(Context, UserId, InputDeviceId, FGamepadKeyNames::SpecialRight, FrameInput.bStart);
	CheckButtonInput(Context, UserId, InputDeviceId, FGamepadKeyNames::SpecialLeft, FrameInput.bShare);

	CheckButtonInput(Context, UserId, InputDeviceId, FGamepadKeyNames::LeftTriggerThreshold, FrameInput.bLeftTriggerThreshold);
	CheckButtonInput(Context, UserId, InputDeviceId, FGamepadKeyNames::RightTriggerThreshold, FrameInput.bRightTriggerThreshold);

	// mapped urenal native gamepad Push Stick
	CheckButtonInput(Context, UserId, InputDeviceId, FGamepadKeyNames::LeftThumb, FrameInput.bLeftStick);
	CheckButtonInput(Context, UserId, InputDeviceId, FGamepadKeyNames::RightThumb, FrameInput.bRightStick);

	// Custom map keys
	// 원본은 스틱 누르기·Options·Create 버튼을 표준 키와 PS 전용 키(PS_PushLeftStick, PS_PushRightStick, PS_Menu,
	// PS_Share)로 두 번 보냈다. 두 키를 모두 매핑하면 입력이 중복되므로 표준 키만 보낸다.
	CheckButtonInput(Context, UserId, InputDeviceId, FName("PS_Mic"), FrameInput.bMute);
	CheckButtonInput(Context, UserId, InputDeviceId, FName("PS_TouchButtom"), FrameInput.bTouch);
	CheckButtonInput(Context, UserId, InputDeviceId, FName("PS_Button"), FrameInput.bPSButton);

	if (Context->DeviceType == EDSDeviceType::DualSenseEdge)
	{
		CheckButtonInput(Context, UserId, InputDeviceId, FName("PS_FunctionL"), FrameInput.bFn1);
		CheckButtonInput(Context, UserId, InputDeviceId, FName("PS_FunctionR"), FrameInput.bFn2);
		CheckButtonInput(Context, UserId, InputDeviceId, FName("PS_PaddleL"), FrameInput.bPaddleLeft);
		CheckButtonInput(Context, UserId, InputDeviceId, FName("PS_PaddleR"), FrameInput.bPaddleRight);
	}

	SensorsImpl(Context, FrameInput, UserId, InputDeviceId, DeltaTime);
	TouchpadImpl(Context, FrameInput, UserId, InputDeviceId, DeltaTime);
}

void DeviceManager::CheckButtonInput(FDeviceContext* Context, const FPlatformUserId UserId, const FInputDeviceId InputDeviceId, const FName ButtonName, const bool IsButtonPressed) const
{
	// 원본은 버튼마다, 매 이벤트 발송마다 FName을 std::string으로 변환해 FDeviceContext::ButtonStates를 조회했다.
	// 장치별 FName 집합으로 눌린 버튼만 기록해 문자열 할당을 없앤다.
	TSet<FName>& Pressed = PressedButtons.FindOrAdd(InputDeviceId);
	const bool PreviousState = Pressed.Contains(ButtonName);
	if (IsButtonPressed && !PreviousState)
	{
		MessageHandler.Get().OnControllerButtonPressed(ButtonName, UserId, InputDeviceId, false);
		Pressed.Add(ButtonName);
	}
	else if (!IsButtonPressed && PreviousState)
	{
		MessageHandler.Get().OnControllerButtonReleased(ButtonName, UserId, InputDeviceId, false);
		Pressed.Remove(ButtonName);
	}
}

void DeviceManager::SetDeviceProperty(int32 ControllerId, const FInputDeviceProperty* Property)
{
	if (!Property)
	{
		return;
	}

	// Handle Request_Device_Update from WM_DEVICECHANGE to trigger immediate device scan.
	static const FName RequestDeviceUpdateName(TEXT("Request_Device_Update"));
	if (Property->Name == RequestDeviceUpdateName)
	{
		FDeviceRegistry::RequestImmediateDetection();
		return;
	}

	if (Property->Name == FInputDeviceLightColorProperty::PropertyName())
	{
		const FInputDeviceLightColorProperty* ColorProperty = static_cast<const FInputDeviceLightColorProperty*>(Property);
		SetLightColor(ControllerId, ColorProperty->Color);
	}

	if (Property->Name == FInputDeviceTriggerFeedbackProperty::PropertyName())
	{
		if (const FInputDeviceTriggerFeedbackProperty* FeedbackProperty = static_cast<const FInputDeviceTriggerFeedbackProperty*>(Property))
		{
			EInputDeviceTriggerMask HandMask = FeedbackProperty->AffectedTriggers;
			if (IGamepadTrigger* GamepadTrigger = GetTriggerInterface(ControllerId))
			{
				GamepadTrigger->SetResistance(FeedbackProperty->Position, FeedbackProperty->Strengh, static_cast<EDSGamepadHand>(HandMask));
			}
		}
	}
}

void DeviceManager::SetHapticFeedbackValues(const int32 ControllerId, const int32 Hand, const FHapticFeedbackValues& Values)
{
}

void DeviceManager::SetChannelValues(int32 ControllerId, const FForceFeedbackValues& Values)
{
	if (ISonyGamepad* Gamepad = GetGamepad(ControllerId))
	{
		const float LeftRumble = FMath::Clamp(FMath::Max(Values.LeftLarge, Values.LeftSmall), 0.f, 1.f);
		const float RightRumble = FMath::Clamp(FMath::Max(Values.RightLarge, Values.RightSmall), 0.f, 1.f);
		Gamepad->SetVibration(static_cast<uint8>(LeftRumble * 255.f), static_cast<uint8>(RightRumble * 255.f));
	}
}

void DeviceManager::SetLightColor(const int32 ControllerId, const FColor Color)
{
	if (ISonyGamepad* Gamepad = GetGamepad(ControllerId))
	{
		Gamepad->SetLightbar({Color.R, Color.G, Color.B, Color.A});
	}
}

bool DeviceManager::IsGamepadAttached() const
{
	// 원본은 항상 true를 돌려 패드가 없어도 Slate가 게임패드가 연결된 것으로 판단했다.
	return FDeviceRegistry::HasAnyDevice();
}

void DeviceManager::HandleInputDeviceConnectionChange(EInputDeviceConnectionState NewConnectionState, FPlatformUserId PlatformUserId, FInputDeviceId InputDeviceId)
{
	if (NewConnectionState != EInputDeviceConnectionState::Disconnected)
	{
		return;
	}

	// 원본은 끊긴 장치에 이벤트를 더 보내지 않아, 마지막 스틱·버튼 값이 엔진에 남아 캐릭터가 계속 움직였다.
	// 이 플러그인이 관리하는 장치만 처리한다. KBM·XInput 장치는 라이브러리가 없으므로 건너뛴다.
	ISonyGamepad* Gamepad = FDeviceRegistry::GetLibraryInstance(InputDeviceId);
	if (!Gamepad)
	{
		return;
	}

	if (FDeviceContext* Context = Gamepad->GetMutableDeviceContext())
	{
		FInputContext NeutralInput;
		CheckEvents(Context, NeutralInput, PlatformUserId, InputDeviceId, 0.0f);
	}
	PressedButtons.Remove(InputDeviceId);
}

void DeviceManager::OnUserLoginChangedEvent(bool bLoggedIn, int32 UserId, int32 UserIndex)
{
	const FPlatformUserId PlatformUserId = FPlatformUserId::CreateFromInternalId(UserId);
	if (!bLoggedIn)
	{
		TArray<FInputDeviceId> OutInputDevices;
		OutInputDevices.Reset();

		IPlatformInputDeviceMapper::Get().GetAllInputDevicesForUser(PlatformUserId, OutInputDevices);
		for (const FInputDeviceId& DeviceId : OutInputDevices)
		{
			IPlatformInputDeviceMapper::Get().Internal_MapInputDeviceToUser(DeviceId, PlatformUserId, EInputDeviceConnectionState::Disconnected);
		}
	}
}
