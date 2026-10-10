// Copyright (c) 2025 Rafael Valoto/Publisher. All rights reserved.
// Created for: GamepadCore - Plugin to support DualSense controller on Windows.
// Planned Release Year: 2025
#pragma once
#include "GCore/Interfaces/IDeviceRegistry.h"
#include "GCore/Interfaces/IPlatformHardwareInfo.h"
#include "GCore/Types/ECoreGamepad.h"
#include "GImplementations/Libraries/DualSense/DualSenseLibrary.h"
#include "GImplementations/Libraries/DualShock/DualShockLibrary.h"
#include <ranges>
#include <vector>

namespace GamepadCore
{
	template<typename T>
	concept DeviceRegistryPolicy = requires(T t, typename T::EngineIdType id) {
		typename T::EngineIdType;

		{ t.AllocEngineDevice() } -> std::same_as<typename T::EngineIdType>;

		{ t.DisconnectDevice(id) } -> std::same_as<void>;

		{ t.DispatchNewGamepad(id) } -> std::same_as<void>;

		// 라이브러리마다 입력 리더를 시작·정지한다. 라이브러리 수명과 같은 곳에서 관리해야 정리 순서가 보장된다.
		{ t.StartReader(id, std::shared_ptr<ISonyGamepad>{}) } -> std::same_as<void>;

		{ t.StopReader(id) } -> std::same_as<void>;
	};

	template<typename DeviceRegistryPolicy>
	class TBasicDeviceRegistry : public IDeviceRegistry
	{
		using EngineIdType = typename DeviceRegistryPolicy::EngineIdType;
		std::unordered_map<std::string, typename DeviceRegistryPolicy::EngineIdType> KnownDevicePaths;
		std::unordered_map<std::string, typename DeviceRegistryPolicy::EngineIdType> HistoryDevices;
		std::unordered_map<typename DeviceRegistryPolicy::EngineIdType, std::shared_ptr<ISonyGamepad>, typename DeviceRegistryPolicy::Hasher> LibraryInstances;

		float TimeAccumulator = 0.0f;
		const float DetectionInterval = 1.0f;

	public:
		DeviceRegistryPolicy Policy;

		virtual ~TBasicDeviceRegistry() override = default;

		virtual void PlugAndPlay(float DeltaTime) override
		{
			TimeAccumulator += DeltaTime;
			if (TimeAccumulator < DetectionInterval)
			{
				return;
			}
			TimeAccumulator = 0.0f;

			std::unordered_set<std::string> OrphanPaths;
			OrphanPaths.clear();
			for (const auto& [Path, Key] : KnownDevicePaths)
			{
				OrphanPaths.insert(Path);
			}

			std::vector<FDeviceContext> DetectedDevices;
			DetectedDevices.clear();
			IPlatformHardwareInfo::Get().Detect(DetectedDevices);

			for (const auto& Context : DetectedDevices)
			{
				OrphanPaths.erase(Context.Path);
			}

			// 읽기·쓰기 실패로 핸들이 무효화된 라이브러리도 제거 대상에 넣는다.
			// 원본은 장치 경로가 계속 감지되면 이 라이브러리를 남겨 두어, 일시적인 Bluetooth 끊김 뒤 입력이 멈췄다.
			// 제거한 경로는 아래 감지 루프에서 HistoryDevices의 같은 DeviceId로 다시 생성된다.
			for (const auto& [Path, Key] : KnownDevicePaths)
			{
				ISonyGamepad* Library = GetLibrary(Key);
				if (!Library || !Library->IsConnected())
				{
					OrphanPaths.insert(Path);
				}
			}

			for (const std::string& Path : OrphanPaths)
			{
				auto It = KnownDevicePaths.find(Path);
				if (It != KnownDevicePaths.end())
				{
					EngineIdType DeviceId = It->second;
					RemoveLibraryInstance(DeviceId);
					KnownDevicePaths.erase(It);
				}
			}

			for (auto Context : DetectedDevices)
			{
				// 이미 라이브러리가 있는 장치는 다시 열지 않는다. 원본은 감지 주기마다 CreateHandle을 호출한 뒤
				// CreateLibrary에서 기존 인스턴스를 발견하면 새 Context를 버려, 패드마다 HID 핸들이 매초 누수되었다.
				if (KnownDevicePaths.contains(Context.Path))
				{
					continue;
				}

				Context.Output = FOutputContext();
				if (bool IsCreateHandle = IPlatformHardwareInfo::Get().CreateHandle(&Context))
				{
					CreateLibrary(Context);
				}
			}
		}

		ISonyGamepad* GetLibrary(EngineIdType DeviceId)
		{
			if (LibraryInstances.contains(DeviceId))
			{
				return LibraryInstances[DeviceId].get();
			}
			return nullptr;
		}

		void RemoveLibraryInstance(EngineIdType DeviceId)
		{
			Policy.DisconnectDevice(DeviceId);
			if (LibraryInstances.contains(DeviceId))
			{
				// 리더가 읽기 중일 수 있으므로 핸들을 닫기 전에 먼저 멈춘다.
				Policy.StopReader(DeviceId);
				LibraryInstances[DeviceId]->ShutdownLibrary();
				LibraryInstances.erase(DeviceId);
			}
		}

		void RequestImmediateDetection()
		{
			TimeAccumulator = DetectionInterval;
		}

	private:
		void CreateLibrary(FDeviceContext& Context)
		{
			std::shared_ptr<ISonyGamepad> Gamepad = nullptr;
			if (Context.DeviceType == EDSDeviceType::DualSense || Context.DeviceType == EDSDeviceType::DualSenseEdge)
			{
				Gamepad = std::make_shared<FDualSenseLibrary>();
			}

			if (Context.DeviceType == EDSDeviceType::DualShock4)
			{
				Gamepad = std::make_shared<FDualShockLibrary>();
			}

			if (!Gamepad)
			{
				return;
			}

			if (!HistoryDevices.contains(Context.Path))
			{
				HistoryDevices[Context.Path] = Policy.AllocEngineDevice();
			}

			auto DeviceId = HistoryDevices[Context.Path];
			if (!LibraryInstances.contains(DeviceId))
			{
				Gamepad->Initialize(Context);
				LibraryInstances[DeviceId] = Gamepad;
				KnownDevicePaths[Context.Path] = DeviceId;
				Policy.DispatchNewGamepad(DeviceId);
				Policy.StartReader(DeviceId, Gamepad);
			}
		}
	};
} // namespace GamepadCore
