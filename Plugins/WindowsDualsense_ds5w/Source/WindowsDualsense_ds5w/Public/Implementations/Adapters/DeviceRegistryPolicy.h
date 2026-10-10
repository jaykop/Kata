// Copyright (c) 2025 Rafael Valoto. All Rights Reserved.
// Project: GamepadCore - Adapter Example
// Description: Example implementation of DeviceRegistry Policy for Unreal Engine.
#pragma once
#include "CoreMinimal.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "GenericPlatform/IInputInterface.h"
#include "InputCoreTypes.h"
#include "Misc/CoreDelegates.h"
#include "Runtime/Launch/Resources/Version.h"
#include <memory>

class ISonyGamepad;
class FSonyGamepadReader;

struct FDeviceRegistryPolicy
{
public:
	using EngineIdType = FInputDeviceId;

	// 리더 스레드를 소유하므로 복사하지 않는다. 생성자·소멸자는 FSonyGamepadReader가 완전한 형식인 cpp에서 정의한다.
	FDeviceRegistryPolicy();
	~FDeviceRegistryPolicy();
	FDeviceRegistryPolicy(const FDeviceRegistryPolicy&) = delete;
	FDeviceRegistryPolicy& operator=(const FDeviceRegistryPolicy&) = delete;

	/**
	 * 새 라이브러리의 입력 리더 스레드를 시작한다. 게임 스레드에서 호출한다.
	 * 같은 장치의 리더가 이미 있으면 먼저 멈춘다.
	 */
	void StartReader(EngineIdType GamepadId, const std::shared_ptr<ISonyGamepad>& Gamepad);

	/** 장치의 입력 리더 스레드를 멈추고 끝날 때까지 기다린다. 리더가 없으면 아무것도 하지 않는다. */
	void StopReader(EngineIdType GamepadId);

	struct Hasher
	{
		std::size_t operator()(const FInputDeviceId& Id) const
		{
			return GetTypeHash(Id);
		}
	};

	FInputDeviceId AllocEngineDevice()
	{
		// We should never call into IPlatformInputDeviceMapper from non-game thread because it is not thread-safe
		check(IsInGameThread());
		return IPlatformInputDeviceMapper::Get().AllocateNewInputDeviceId();
	}

	/**
	 * 장치를 FInputDeviceRegistry에 등록한다. 하드웨어 식별자가 UInputPlatformSettings에 없으면 Gamepad 유형으로 추가한다.
	 * UE 5.8에서 FInputDeviceScope는 deprecated이고 생성자가 장치를 등록하지 않아, 원본 방식으로는
	 * UInputDeviceSubsystem이 듀얼센스를 최근 사용 장치로 식별하지 못했다. 게임 스레드에서 호출한다.
	 */
	static void RegisterDeviceDescriptor(EngineIdType GamepadId, const std::shared_ptr<ISonyGamepad>& Gamepad);

	void DispatchNewGamepad(EngineIdType GamepadId, const std::shared_ptr<ISonyGamepad>& Gamepad)
	{
		check(IsInGameThread());
		// 연결을 알리기 전에 등록해야 Internal_MapInputDeviceToUser가 OnInputDeviceHardwareInfoAvailable도 함께 보낸다.
		RegisterDeviceDescriptor(GamepadId, Gamepad);
		if (IPlatformInputDeviceMapper::Get().GetInputDeviceConnectionState(GamepadId) != EInputDeviceConnectionState::Connected)
		{
			FPlatformUserId UserId = IPlatformInputDeviceMapper::Get().GetUserForInputDevice(GamepadId);
			if (!UserId.IsValid())
			{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 6
				TArray<FInputDeviceId> Devices;
				Devices.Reset();

				IPlatformInputDeviceMapper::Get().GetAllInputDevicesForUser(
				    IPlatformInputDeviceMapper::Get().GetPrimaryPlatformUser(), Devices);

				bool AllocateDeviceToDefaultUser = false;
				if (Devices.Num() <= 1)
				{
					AllocateDeviceToDefaultUser = true;
				}

				UserId = AllocateDeviceToDefaultUser
				             ? IPlatformInputDeviceMapper::Get().GetPrimaryPlatformUser()
				             : IPlatformInputDeviceMapper::Get().AllocateNewUserId();

#else
				UserId = IPlatformInputDeviceMapper::Get().GetPlatformUserForNewlyConnectedDevice();
#endif
			}

			IPlatformInputDeviceMapper::Get().Internal_MapInputDeviceToUser(GamepadId, UserId, EInputDeviceConnectionState::Connected);
		}
	}

	static void DisconnectDevice(EngineIdType GamepadId)
	{
		IPlatformInputDeviceMapper::Get().Internal_SetInputDeviceConnectionState(GamepadId, EInputDeviceConnectionState::Disconnected);
	}

	static FInputDeviceId ConvertFromInt(int32 Id)
	{
		return FInputDeviceId::CreateFromInternalId(Id);
	}

	static int32 ConvertToInt(FInputDeviceId Id)
	{
		return Id.GetId();
	}

private:
	/** 장치별 입력 리더. 레지스트리가 파괴될 때 이 정책의 소멸자가 모두 멈춘다. */
	TMap<FInputDeviceId, TUniquePtr<FSonyGamepadReader>> Readers;
};
