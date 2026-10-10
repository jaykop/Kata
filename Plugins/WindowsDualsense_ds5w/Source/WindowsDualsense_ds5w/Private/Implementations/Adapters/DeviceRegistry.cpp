// Copyright (c) 2025 Rafael Valoto/Publisher. All rights reserved.
// Created for: WindowsDualsense_ds5w - Plugin to support DualSense controller on Windows.
// Planned Release Year: 2025

#include "Implementations/Adapters/DeviceRegistry.h"
#include "GCore/Interfaces/ISonyGamepad.h"

TUniquePtr<FDeviceRegistry> FDeviceRegistry::Instance;
std::unique_ptr<FDeviceRegistry::FRegistryLogic> FDeviceRegistry::RegistryImplementation = nullptr;

FDeviceRegistry::~FDeviceRegistry()
{
}

FDeviceRegistry::FDeviceRegistry()
{
	RegistryImplementation = std::make_unique<FRegistryLogic>();
}

void FDeviceRegistry::Initialize()
{
	if (!Instance)
	{
		check(IsInGameThread());
		Instance.Reset(new FDeviceRegistry());
	}
}

void FDeviceRegistry::Shutdown()
{
	Instance.Reset();
	// 원본은 RegistryImplementation을 정적 소멸 시점까지 남겼다. 이제 레지스트리 정책이 입력 리더 스레드를 소유하므로,
	// 모듈 종료 시점에 명시적으로 파괴해 스레드를 멈춘다.
	RegistryImplementation.reset();
}

FDeviceRegistry* FDeviceRegistry::Get()
{
	check(IsInGameThread());
	return Instance.Get();
}

void FDeviceRegistry::DiscoverDevices(float DeltaTime)
{
	if (RegistryImplementation)
	{
		return RegistryImplementation->PlugAndPlay(DeltaTime);
	}
}

ISonyGamepad* FDeviceRegistry::GetLibraryInstance(FInputDeviceId DeviceId)
{
	// 순수 조회만 한다. 원본은 조회에 실패하면 RemoveLibraryInstance로 장치를 끊김 처리했다.
	// 호출자는 KBM·XInput을 포함해 연결된 모든 장치를 이 함수로 조회하므로, Sony 패드가 아닌 장치까지
	// Disconnected로 바뀌었다. 라이브러리 제거와 끊김 처리는 TBasicDeviceRegistry::PlugAndPlay에서만 한다.
	if (RegistryImplementation)
	{
		return RegistryImplementation->GetLibrary(DeviceId);
	}
	return nullptr;
}

bool FDeviceRegistry::HasAnyDevice()
{
	return RegistryImplementation && RegistryImplementation->HasLibraries();
}

void FDeviceRegistry::RequestImmediateDetection()
{
	if (RegistryImplementation)
	{
		RegistryImplementation->RequestImmediateDetection();
	}
}
