// Copyright (c) 2025 Rafael Valoto. All Rights Reserved.
// Project: GamepadCore
// Description: Cross-platform library for DualSense and generic gamepad input support.
// Targets: Windows, Linux, macOS.
#pragma once
#include "GCore/Templates/TGenericHardwareInfo.h"
#include "WindowsDeviceInfo.h"
#ifdef _WIN32
namespace FWindowsPlatform
{
	struct FWindowsHardwarePolicy;
	using FWindowsHardware = GamepadCore::TGenericHardwareInfo<FWindowsHardwarePolicy>;

	struct FWindowsHardwarePolicy
	{
	public:
		void Read(FDeviceContext* Context)
		{
			FWindowsDeviceInfo::Read(Context);
		}

		void Write(FDeviceContext* Context)
		{
			FWindowsDeviceInfo::Write(Context);
		}

		void Detect(std::vector<FDeviceContext>& Devices)
		{
			FWindowsDeviceInfo::Detect(Devices);
		}

		bool CreateHandle(FDeviceContext* Context)
		{
			return FWindowsDeviceInfo::CreateHandle(Context);
		}

		void InvalidateHandle(FDeviceContext* Context)
		{
			FWindowsDeviceInfo::InvalidateHandle(Context);
		}

		void ProcessAudioHaptic(FDeviceContext* Context)
		{
			FWindowsDeviceInfo::ProcessAudioHaptic(Context);
		}

		void InitializeAudioDevice(FDeviceContext* Context)
		{
			FWindowsDeviceInfo::InitializeAudioDevice(Context);
		}
	};
} // namespace FWindowsPlatform
#endif
