// Copyright (c) 2025 Rafael Valoto/Publisher. All rights reserved.
// Created for: WindowsDualsense_ds5w - Plugin to support DualSense controller on Windows.
// Planned Release Year: 2025

#include "Implementations/Platforms/Windows/WindowsDeviceInfo.h"
#if PLATFORM_WINDOWS
#include "GCore/Types/Structs/Config/GamepadCalibration.h"
#include "GImplementations/Utils/GamepadSensors.h"
#include "Helpers/DualSenseLog.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <hidsdi.h>
#include <setupapi.h>

using namespace FGamepadAudio;

void FWindowsDeviceInfo::Detect(std::vector<FDeviceContext>& Devices)
{
	Devices.clear();

	GUID HidGuid;
	HidD_GetHidGuid(&HidGuid);
	const HDEVINFO DeviceInfoSet = SetupDiGetClassDevs(&HidGuid, nullptr, nullptr,
	                                                   DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if (DeviceInfoSet == INVALID_HANDLE_VALUE)
	{
		return;
	}

	SP_DEVICE_INTERFACE_DATA DeviceInterfaceData = {};
	DeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
	for (std::int32_t DeviceIndex = 0; SetupDiEnumDeviceInterfaces(DeviceInfoSet, nullptr, &HidGuid, DeviceIndex,
	                                                               &DeviceInterfaceData);
	     DeviceIndex++)
	{
		DWORD RequiredSize = 0;
		SetupDiGetDeviceInterfaceDetail(DeviceInfoSet, &DeviceInterfaceData, nullptr, 0, &RequiredSize, nullptr);

		const auto DetailDataBuffer = static_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA>(malloc(RequiredSize));
		if (!DetailDataBuffer)
		{
			UE_LOG(LogDualSense, Error, TEXT("HIDManager: Failed to allocate memory for device details."));
			continue;
		}

		DetailDataBuffer->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
		if (SetupDiGetDeviceInterfaceDetail(DeviceInfoSet, &DeviceInterfaceData, DetailDataBuffer, RequiredSize,
		                                    nullptr, nullptr))
		{
			std::string InPath = std::filesystem::path(DetailDataBuffer->DevicePath).string();

			// Sony 장치 경로만 연다. 원본은 1초마다 키보드·마우스를 포함한 모든 HID 장치를 게임 스레드에서 열었다.
			// 경로의 VID 표기가 USB는 "vid_054c", Bluetooth는 "vid&0002054c"라서 "054c" 포함 여부만 보고,
			// 정확한 판별은 아래 HidD_GetAttributes의 VendorID·ProductID 검사가 맡는다.
			std::string LowerPath = InPath;
			std::transform(LowerPath.begin(), LowerPath.end(), LowerPath.begin(),
			               [](unsigned char Ch) { return static_cast<char>(std::tolower(Ch)); });
			if (LowerPath.find("054c") == std::string::npos)
			{
				free(DetailDataBuffer);
				continue;
			}
			const HANDLE TempDeviceHandle = CreateFileW(
			    DetailDataBuffer->DevicePath,
			    GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, NULL, nullptr);

			if (TempDeviceHandle != INVALID_HANDLE_VALUE)
			{
				HIDD_ATTRIBUTES Attributes = {};
				Attributes.Size = sizeof(HIDD_ATTRIBUTES);
				if (HidD_GetAttributes(TempDeviceHandle, &Attributes))
				{
					if (
					    Attributes.VendorID == 0x054C &&
					    (Attributes.ProductID == 0x0CE6 ||
					     Attributes.ProductID == 0x0DF2 ||
					     Attributes.ProductID == 0x05C4 ||
					     Attributes.ProductID == 0x09CC))
					{
						FDeviceContext Context = {};
						Context.Handle = nullptr;
						Context.Path = std::filesystem::path(DetailDataBuffer->DevicePath).string();
						switch (Attributes.ProductID)
						{
							case 0x05C4:
							case 0x09CC:
								Context.DeviceType = EDSDeviceType::DualShock4;
								break;
							case 0x0CE6:
								Context.DeviceType = EDSDeviceType::DualSense;
								break;
							case 0x0DF2:
								Context.DeviceType = EDSDeviceType::DualSenseEdge;
								break;
							default:
								Context.DeviceType = EDSDeviceType::NotFound;
						}

						if (Context.DeviceType != EDSDeviceType::NotFound)
						{
							Context.IsConnected = true;
							Context.ConnectionType = EDSDeviceConnection::Usb;
							std::string BluetoothGUID = "{00001124-0000-1000-8000-00805f9b34fb}";
							if (Context.Path.find(BluetoothGUID) != std::string::npos)
							{
								Context.ConnectionType = EDSDeviceConnection::Bluetooth;
							}
							Devices.push_back(Context);
						}
					}
				}
				if (TempDeviceHandle != INVALID_HANDLE_VALUE)
				{
					CloseHandle(TempDeviceHandle);
				}
			}
		}
		free(DetailDataBuffer);
	}
	SetupDiDestroyDeviceInfoList(DeviceInfoSet);
}

void FWindowsDeviceInfo::Read(FDeviceContext* Context)
{
	if (!Context)
	{
		return;
	}

	if (Context->Handle == INVALID_PLATFORM_HANDLE)
	{
		return;
	}

	if (!Context->IsConnected)
	{
		return;
	}

	DWORD BytesRead = 0;
	if (Context->ConnectionType == EDSDeviceConnection::Bluetooth && Context->DeviceType == EDSDeviceType::DualShock4)
	{
		constexpr size_t InputReportLength = 547;
		EPollResult Result = PollTick(Context->Handle, Context->BufferDS4, InputReportLength, BytesRead);
		if (Result != EPollResult::ReadOk)
		{
			InvalidateHandle(Context);
		}
	}
	else
	{
		const size_t InputBufferSize = Context->ConnectionType == EDSDeviceConnection::Bluetooth ? 78 : 64;

		// Flush all queued reports from HID buffer to prevent input lag.
		if (Context->ConnectionType == EDSDeviceConnection::Usb)
		{
			HidD_FlushQueue(Context->Handle);
		}

		EPollResult Result = PollTick(Context->Handle, Context->Buffer, InputBufferSize, BytesRead);
		if (Result != EPollResult::ReadOk)
		{
			InvalidateHandle(Context);
		}
	}
}

void FWindowsDeviceInfo::Write(FDeviceContext* Context)
{
	if (Context->Handle == INVALID_HANDLE_VALUE)
	{
		return;
	}

	size_t InReportLength = Context->DeviceType == EDSDeviceType::DualShock4 ? 32 : 74;
	size_t OutputReportLength = Context->ConnectionType == EDSDeviceConnection::Bluetooth ? 78 : InReportLength;

	DWORD BytesWritten = 0;
	if (!WriteFile(Context->Handle, Context->GetRawOutputBuffer(), OutputReportLength, &BytesWritten, nullptr))
	{
		InvalidateHandle(Context);
	}
}

bool FWindowsDeviceInfo::CreateHandle(FDeviceContext* DeviceContext)
{
	std::string Source = DeviceContext->Path;
	std::wstring MyStdString = std::filesystem::path(Source).wstring();
	const HANDLE DeviceHandle = CreateFileW(
	    MyStdString.data(),
	    GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, NULL, nullptr);

	if (DeviceHandle == INVALID_PLATFORM_HANDLE)
	{
		DeviceContext->Handle = DeviceHandle;
		return false;
	}

	HANDLE DuplicatedHandle = INVALID_HANDLE_VALUE;
	if (DuplicateHandle(GetCurrentProcess(), DeviceHandle, GetCurrentProcess(), &DuplicatedHandle, 0, 0, DUPLICATE_SAME_ACCESS))
	{
		CloseHandle(DeviceHandle);
		DeviceContext->Handle = DuplicatedHandle;
	}
	else
	{
		DeviceContext->Handle = DeviceHandle;
	}

	ConfigureFeatures(DeviceContext);
	return true;
}

void FWindowsDeviceInfo::InvalidateHandle(FDeviceContext* Context)
{
	if (!Context)
	{
		return;
	}

	if (Context->Handle != INVALID_PLATFORM_HANDLE)
	{
		CloseHandle(Context->Handle);
		Context->Handle = INVALID_PLATFORM_HANDLE;
		Context->IsConnected = false;
		Context->Path.clear();

		unsigned char* RawOutput = Context->GetRawOutputBuffer();
		std::memset(RawOutput, 0, 78);
		std::memset(Context->Buffer, 0, 78);
		std::memset(Context->BufferDS4, 0, 547);
		std::memset(Context->BufferAudio, 0, 142);
	}
}

EPollResult FWindowsDeviceInfo::PollTick(HANDLE Handle, unsigned char* Buffer, std::int32_t Length, DWORD& OutBytesRead)
{
	std::int32_t Err = ERROR_SUCCESS;
	PingOnce(Handle, &Err);

	OutBytesRead = 0;
	if (!ReadFile(Handle, Buffer, Length, &OutBytesRead, nullptr))
	{
		return EPollResult::Disconnected;
	}

	return EPollResult::ReadOk;
}

/**
 * @brief Initializes the audio device for a DualSense controller.
 *
 * Enumerates available audio playback devices, searches for one matching
 * the DualSense controller (by name containing "DualSense" or "Wireless Controller"),
 * and initializes the FAudioDeviceContext with the found device.
 *
 * @param Context The device context to store the audio device in
 */
void FWindowsDeviceInfo::InitializeAudioDevice(FDeviceContext* Context)
{
	if (!Context)
	{
		return;
	}

	// Initialize miniaudio context for device enumeration
	ma_context maContext;
	if (ma_context_init(nullptr, 0, nullptr, &maContext) != MA_SUCCESS)
	{
		return;
	}

	// Get playback devices
	ma_device_info* pPlaybackInfos;
	ma_uint32 playbackCount;
	ma_device_info* pCaptureInfos;
	ma_uint32 captureCount;

	if (ma_context_get_devices(&maContext, &pPlaybackInfos, &playbackCount, &pCaptureInfos, &captureCount) != MA_SUCCESS)
	{
		ma_context_uninit(&maContext);
		return;
	}

	// Search for DualSense audio device
	ma_device_id* pFoundDeviceId = nullptr;
	ma_device_id foundDeviceId;

	for (ma_uint32 i = 0; i < playbackCount; i++)
	{
		std::string deviceName(pPlaybackInfos[i].name);

		// Check if device name contains DualSense identifiers
		// DualSense appears as "Wireless Controller" or "DualSense Wireless Controller"
		if (deviceName.find("DualSense") != std::string::npos ||
		    deviceName.find("Wireless Controller") != std::string::npos)
		{
			// Verify it has 4 channels (stereo + haptic L/R)
			// DualSense audio device typically has 4 channels for haptics
			foundDeviceId = pPlaybackInfos[i].id;
			pFoundDeviceId = &foundDeviceId;
			break;
		}
	}

	// Initialize audio context with found device (or default if not found)
	Context->AudioContext = std::make_shared<FAudioDeviceContext>();

	if (pFoundDeviceId)
	{
		// Initialize with specific DualSense device
		// DualSense haptics use 4 channels at 48000 Hz
		Context->AudioContext->InitializeWithDeviceId(pFoundDeviceId, 48000, 4);
	}

	ma_context_uninit(&maContext);
}

bool FWindowsDeviceInfo::PingOnce(HANDLE Handle, std::int32_t* OutLastError)
{
	FILE_STANDARD_INFO Info{};
	if (!GetFileInformationByHandleEx(Handle, FileStandardInfo, &Info, sizeof(Info)))
	{
		if (OutLastError)
		{
			*OutLastError = GetLastError();
		}
		return false;
	}
	if (OutLastError)
	{
		*OutLastError = ERROR_SUCCESS;
	}
	return true;
}

void FWindowsDeviceInfo::ProcessAudioHaptic(FDeviceContext* Context)
{
	if (!Context || !Context->Handle)
	{
		return;
	}

	if (Context->Handle == INVALID_PLATFORM_HANDLE)
	{
		return;
	}

	if (Context->ConnectionType != EDSDeviceConnection::Bluetooth)
	{
		return;
	}

	unsigned long BytesWritten = 0;
	constexpr size_t BufferSize = 142;
	if (!WriteFile(Context->Handle, Context->BufferAudio, BufferSize, &BytesWritten, nullptr))
	{
		const unsigned long Error = GetLastError();
		if (Error != ERROR_IO_PENDING)
		{
		}
	}
}

void FWindowsDeviceInfo::ConfigureFeatures(FDeviceContext* Context)
{
	using namespace FGamepadSensors;
	FGamepadCalibration Calibration;

	if (Context->DeviceType == EDSDeviceType::DualShock4)
	{
		if (Context->ConnectionType == EDSDeviceConnection::Usb)
		{
			unsigned char FeatureBuffer[37] = {0};
			std::memset(FeatureBuffer, 0, sizeof(FeatureBuffer));

			FeatureBuffer[0] = 0x02;
			if (!HidD_GetFeature(Context->Handle, FeatureBuffer, 37))
			{
				const unsigned long Error = GetLastError();
				return;
			}

			DualShockCalibrationSensors(FeatureBuffer, Calibration, Context->ConnectionType);
		}
		else
		{

			unsigned char FeatureBuffer[41] = {0};
			std::memset(FeatureBuffer, 0, sizeof(FeatureBuffer));

			FeatureBuffer[0] = 0x05;
			if (!HidD_GetFeature(Context->Handle, FeatureBuffer, 41))
			{
				const unsigned long Error = GetLastError();
				return;
			}

			DualShockCalibrationSensors(FeatureBuffer, Calibration, Context->ConnectionType);
		}

		Context->Calibration = Calibration;
	}
	else
	{
		unsigned char FeatureBuffer[41] = {0};
		std::memset(FeatureBuffer, 0, sizeof(FeatureBuffer));

		FeatureBuffer[0] = 0x05;
		if (!HidD_GetFeature(Context->Handle, FeatureBuffer, 41))
		{
			const unsigned long Error = GetLastError();
			return;
		}

		DualSenseCalibrationSensors(FeatureBuffer, Calibration);
		Context->Calibration = Calibration;
	}
}

#endif PLATFORM_WINDOWS
