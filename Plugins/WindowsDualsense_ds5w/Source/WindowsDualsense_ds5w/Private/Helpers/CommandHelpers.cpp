// Copyright (c) 2025 Rafael Valoto/Publisher. All rights reserved.
// Created for: WindowsDualsense_ds5w - Plugin to support DualSense controller on Windows.
// Planned Release Year: 2025

#include "Helpers/CommandHelpers.h"
#include "API/SonyGamepadProxyHelpers.h"
#include "GCore/Interfaces/ISonyGamepad.h"
#include "GCore/Types/Structs/Context/DeviceContext.h"
#include "GImplementations/Utils/GamepadOutput.h"
#include "HAL/IConsoleManager.h"

using namespace SonyGamepadProxyHelpers;

static FAutoConsoleCommand GCmd_SetAudioByte(
    TEXT("ds.SetAudioByte"),
    TEXT("ds.SetAudioByte <DeviceId> <Index 0-9> <Value 0-255>"),
    FConsoleCommandWithArgsDelegate::CreateStatic(&FCommandHelpers::HandleSetAudioByte));
static FAutoConsoleCommand GCmd_SetAudioLR(
    TEXT("ds.SetAudioLR"),
    TEXT("ds.SetAudioLR <DeviceId> <L1> <L2> <R1> <R2> <Master> (0-255)"),
    FConsoleCommandWithArgsDelegate::CreateStatic(&FCommandHelpers::HandleSetAudioLR));
static FAutoConsoleCommand GCmd_DumpAudioBytes(
    TEXT("ds.DumpAudioBytes"),
    TEXT("ds.DumpAudioBytes <DeviceId>"),
    FConsoleCommandWithArgsDelegate::CreateStatic(&FCommandHelpers::HandleDumpAudioBytes));
static FAutoConsoleCommand GCmd_SetTrigR(
    TEXT("ds.SetTrigR"),
    TEXT("ds.SetTrigR <DeviceId> <hex bytes up to 10> e.g. 22 3F 08 01"),
    FConsoleCommandWithArgsDelegate::CreateStatic(&FCommandHelpers::HandleSetTrigR));
static FAutoConsoleCommand GCmd_SetTrigL(
    TEXT("ds.SetTrigL"),
    TEXT("ds.SetTrigL <DeviceId> <hex bytes up to 10> e.g. 22 3F 08 01"),
    FConsoleCommandWithArgsDelegate::CreateStatic(&FCommandHelpers::HandleSetTrigL));
static FAutoConsoleCommand GCmd_DumpTrig(
    TEXT("ds.DumpTrig"),
    TEXT("ds.DumpTrig <DeviceId>"),
    FConsoleCommandWithArgsDelegate::CreateStatic(&FCommandHelpers::HandleDumpTrig));
static FAutoConsoleCommand GCmd_ClearTrig(
    TEXT("ds.ClearTrig"),
    TEXT("ds.ClearTrig <DeviceId>"),
    FConsoleCommandWithArgsDelegate::CreateStatic(&FCommandHelpers::HandleClearTrig));
static FAutoConsoleCommand GCmd_BowR(
    TEXT("ds.BowR"),
    TEXT("ds.BowR <DeviceId> <Start 0-7> <End 0-8> <ResistancePos 0-8> <ForcePos 0-8>"),
    FConsoleCommandWithArgsDelegate::CreateStatic(&FCommandHelpers::HandleBowTrigR));
static FAutoConsoleCommand GCmd_BowL(
    TEXT("ds.BowL"),
    TEXT("ds.BowL <DeviceId> <Start 0-7> <End 0-8> <ResistancePos 0-8> <ForcePos 0-8>"),
    FConsoleCommandWithArgsDelegate::CreateStatic(&FCommandHelpers::HandleBowTrigL));
static FAutoConsoleCommand GCmd_GallopR(
    TEXT("ds.GallopR"),
    TEXT("ds.GallopR <DeviceId> <Start 0-8> <End 1-9> <FirstFoot 0-8> <SecondFoot 1-9> <Freq 0-255>"),
    FConsoleCommandWithArgsDelegate::CreateStatic(&FCommandHelpers::HandleGallopTrigR));
static FAutoConsoleCommand GCmd_GallopL(
    TEXT("ds.GallopL"),
    TEXT("ds.GallopL <DeviceId> <Start 0-8> <End 1-9> <FirstFoot 0-8> <SecondFoot 1-9> <Freq 0-255>"),
    FConsoleCommandWithArgsDelegate::CreateStatic(&FCommandHelpers::HandleGallopTrigL));

void FCommandHelpers::Register()
{
	/* static commands auto-register */
}

bool FCommandHelpers::ParseDeviceId(const TArray<FString>& Args, FInputDeviceId& OutDeviceId)
{
	if (Args.Num() < 1)
	{
		UE_LOG(LogDualSense, Warning, TEXT("Missing <DeviceId> as first argument"));
		return false;
	}
	int32 Id = FCString::Atoi(*Args[0]);
	OutDeviceId = FInputDeviceId::CreateFromInternalId(Id);
	if (!OutDeviceId.IsValid())
	{
		UE_LOG(LogDualSense, Warning, TEXT("Invalid DeviceId: %d"), Id);
		return false;
	}
	return true;
}

bool FCommandHelpers::ParseHexByte(const FString& Token, uint8& OutByte)
{
	FString S = Token.TrimStartAndEnd();
	if (S.StartsWith(TEXT("0x"), ESearchCase::IgnoreCase))
	{
		S.RightChopInline(2);
	}
	if (S.IsEmpty())
	{
		OutByte = 0;
		return false;
	}
	for (int32 i = 0; i < S.Len(); ++i)
	{
		TCHAR C = S[i];
		if (!((C >= '0' && C <= '9') || (C >= 'a' && C <= 'f') || (C >= 'A' && C <= 'F')))
		{
			OutByte = 0;
			return false;
		}
	}
	int32 Value = 0;
	for (int32 i = 0; i < S.Len(); ++i)
	{
		TCHAR C = S[i];
		int32 Nibble = 0;
		if (C >= '0' && C <= '9')
		{
			Nibble = C - '0';
		}
		else if (C >= 'a' && C <= 'f')
		{
			Nibble = 10 + (C - 'a');
		}
		else if (C >= 'A' && C <= 'F')
		{
			Nibble = 10 + (C - 'A');
		}
		Value = (Value << 4) | Nibble;
	}
	OutByte = ClampByte(Value);
	return true;
}

void FCommandHelpers::HandleSetAudioByte(const TArray<FString>& Args)
{
	FInputDeviceId DeviceId;
	if (!ParseDeviceId(Args, DeviceId))
	{
		return;
	}
	ISonyGamepad* Gamepad = GetGamepad(DeviceId.GetId());
	if (!Gamepad)
	{
		return;
	}
	FDeviceContext* Ctx = Gamepad->GetMutableDeviceContext();
	if (!Ctx || !Ctx->IsConnected)
	{
		UE_LOG(LogDualSense, Warning, TEXT("Device not ready/connected"));
		return;
	}
	if (Args.Num() < 3)
	{
		UE_LOG(LogDualSense, Warning, TEXT("Usage: ds.SetAudioByte <DeviceId> <Index 0-9> <Value 0-255>"));
		return;
	}
	int32 Index = FCString::Atoi(*Args[1]);
	int32 Value = FCString::Atoi(*Args[2]);
	if (Index < 0 || Index > 9)
	{
		UE_LOG(LogDualSense, Warning, TEXT("Index out of range (0-9)"));
		return;
	}
	Ctx->BufferAudio[Index] = ClampByte(Value);
	UE_LOG(LogDualSense, Log, TEXT("Audio byte[%d] = %d"), Index, (int32)Ctx->BufferAudio[Index]);
	FGamepadOutput::OutputDualSense(Ctx);
}

void FCommandHelpers::HandleSetAudioLR(const TArray<FString>& Args)
{
	FInputDeviceId DeviceId;
	if (!ParseDeviceId(Args, DeviceId))
	{
		return;
	}
	ISonyGamepad* Gamepad = GetGamepad(DeviceId.GetId());
	if (!Gamepad)
	{
		return;
	}
	FDeviceContext* Ctx = Gamepad->GetMutableDeviceContext();
	if (!Ctx || !Ctx->IsConnected)
	{
		UE_LOG(LogDualSense, Warning, TEXT("Device not ready/connected"));
		return;
	}
	if (Args.Num() < 6)
	{
		UE_LOG(LogDualSense, Warning, TEXT("Usage: ds.SetAudioLR <DeviceId> <L1> <L2> <R1> <R2> <Master>"));
		return;
	}
	int32 L1 = FCString::Atoi(*Args[1]);
	int32 L2 = FCString::Atoi(*Args[2]);
	int32 R1 = FCString::Atoi(*Args[3]);
	int32 R2 = FCString::Atoi(*Args[4]);
	int32 X = FCString::Atoi(*Args[5]);
	Ctx->BufferAudio[5] = ClampByte(L1);
	Ctx->BufferAudio[6] = ClampByte(L2);
	Ctx->BufferAudio[7] = ClampByte(R1);
	Ctx->BufferAudio[8] = ClampByte(R2);
	Ctx->BufferAudio[9] = ClampByte(X);
	UE_LOG(LogDualSense, Log, TEXT("Audio [5..9] = %d, %d, %d, %d, %d"), (int32)Ctx->BufferAudio[5], (int32)Ctx->BufferAudio[6], (int32)Ctx->BufferAudio[7], (int32)Ctx->BufferAudio[8], (int32)Ctx->BufferAudio[9]);
	FGamepadOutput::OutputDualSense(Ctx);
}

void FCommandHelpers::HandleDumpAudioBytes(const TArray<FString>& Args)
{
	FInputDeviceId DeviceId;
	if (!ParseDeviceId(Args, DeviceId))
	{
		return;
	}
	ISonyGamepad* Gamepad = GetGamepad(DeviceId.GetId());
	if (!Gamepad)
	{
		return;
	}
	FDeviceContext* Ctx = Gamepad->GetMutableDeviceContext();
	if (!Ctx || !Ctx->IsConnected)
	{
		UE_LOG(LogDualSense, Warning, TEXT("Device not ready/connected"));
		return;
	}
	for (int32 i = 0; i < 10; ++i)
	{
		UE_LOG(LogDualSense, Log, TEXT("Audio byte[%d] = %d"), i, (int32)Ctx->BufferAudio[i]);
	}
}

void FCommandHelpers::HandleSetTrigR(const TArray<FString>& Args)
{
	FInputDeviceId DeviceId;
	if (!ParseDeviceId(Args, DeviceId))
	{
		return;
	}
	ISonyGamepad* Gamepad = GetGamepad(DeviceId.GetId());
	if (!Gamepad)
	{
		return;
	}
	FDeviceContext* Ctx = Gamepad->GetMutableDeviceContext();
	if (!Ctx || !Ctx->IsConnected)
	{
		UE_LOG(LogDualSense, Warning, TEXT("Device not ready/connected"));
		return;
	}
	FMemory::Memset(Ctx->OverrideTriggerRight, 0, 10);
	for (int32 i = 0; i < 10 && (i + 1) < Args.Num(); ++i)
	{
		uint8 B = 0;
		if (!ParseHexByte(Args[i + 1], B))
		{
			UE_LOG(LogDualSense, Warning, TEXT("Invalid hex at pos %d"), i);
			return;
		}
		Ctx->OverrideTriggerRight[i] = B;
	}
	Ctx->bOverrideTriggerBytes = true;
	UE_LOG(LogDualSense, Log, TEXT("Right trigger override updated."));
	FGamepadOutput::OutputDualSense(Ctx);
}

void FCommandHelpers::HandleSetTrigL(const TArray<FString>& Args)
{
	FInputDeviceId DeviceId;
	if (!ParseDeviceId(Args, DeviceId))
	{
		return;
	}
	ISonyGamepad* Gamepad = GetGamepad(DeviceId.GetId());
	if (!Gamepad)
	{
		return;
	}
	FDeviceContext* Ctx = Gamepad->GetMutableDeviceContext();
	if (!Ctx || !Ctx->IsConnected)
	{
		UE_LOG(LogDualSense, Warning, TEXT("Device not ready/connected"));
		return;
	}
	FMemory::Memset(Ctx->OverrideTriggerLeft, 0, 10);
	for (int32 i = 0; i < 10 && (i + 1) < Args.Num(); ++i)
	{
		uint8 B = 0;
		if (!ParseHexByte(Args[i + 1], B))
		{
			UE_LOG(LogDualSense, Warning, TEXT("Invalid hex at pos %d"), i);
			return;
		}
		Ctx->OverrideTriggerLeft[i] = B;
	}
	Ctx->bOverrideTriggerBytes = true;
	UE_LOG(LogDualSense, Log, TEXT("Left trigger override updated."));
	FGamepadOutput::OutputDualSense(Ctx);
}

void FCommandHelpers::HandleDumpTrig(const TArray<FString>& Args)
{
	FInputDeviceId DeviceId;
	if (!ParseDeviceId(Args, DeviceId))
	{
		return;
	}
	ISonyGamepad* Gamepad = GetGamepad(DeviceId.GetId());
	if (!Gamepad)
	{
		return;
	}
	FDeviceContext* Ctx = Gamepad->GetMutableDeviceContext();
	if (!Ctx || !Ctx->IsConnected)
	{
		UE_LOG(LogDualSense, Warning, TEXT("Device not ready/connected"));
		return;
	}
	char R[10] = {0};
	char L[10] = {0};
	FMemory::Memcpy(R, Ctx->OverrideTriggerRight, 10);
	FMemory::Memcpy(L, Ctx->OverrideTriggerLeft, 10);
	UE_LOG(LogDualSense, Log, TEXT("Dumping OVERRIDE trigger bytes (HEX):"));
	FString RStr, LStr;
	for (int32 i = 0; i < 10; ++i)
	{
		RStr += FString::Printf(TEXT("%02X "), static_cast<uint8>(R[i]));
		LStr += FString::Printf(TEXT("%02X "), static_cast<uint8>(L[i]));
	}
	UE_LOG(LogDualSense, Log, TEXT("R[10..19]: %s"), *RStr);
	UE_LOG(LogDualSense, Log, TEXT("L[21..30]: %s"), *LStr);
}

void FCommandHelpers::HandleClearTrig(const TArray<FString>& Args)
{
	FInputDeviceId DeviceId;
	if (!ParseDeviceId(Args, DeviceId))
	{
		return;
	}
	ISonyGamepad* Gamepad = GetGamepad(DeviceId.GetId());
	if (!Gamepad)
	{
		return;
	}
	FDeviceContext* Ctx = Gamepad->GetMutableDeviceContext();
	if (!Ctx)
	{
		return;
	}
	Ctx->bOverrideTriggerBytes = false;
	FMemory::Memset(Ctx->OverrideTriggerRight, 0, 10);
	FMemory::Memset(Ctx->OverrideTriggerLeft, 0, 10);
	UE_LOG(LogDualSense, Log, TEXT("Trigger overrides cleared."));
	if (Ctx->IsConnected)
	{
		FGamepadOutput::OutputDualSense(Ctx);
	}
}

static bool ComposeBowBytes(uint8 Out[10], int32 StartPosition, int32 EndPosition, int32 ResistancePos, int32 ForcePos)
{
	// Validate ranges
	if (StartPosition < 0 || StartPosition > 7)
	{
		return false;
	}
	if (EndPosition < 0 || EndPosition > 8)
	{
		return false;
	}
	// New spec: Resistance/Force provided in 0..8 positions, convert to 0..255 scale, then to nibble 0..15
	if (ResistancePos < 0 || ResistancePos > 9)
	{
		return false;
	}
	if (ForcePos < 0 || ForcePos > 8)
	{
		return false;
	}

	// Quantize 0..255 -> nibble 0..15 used by protocol
	const uint8 ResistanceNib = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt((ResistancePos / 9.0f) * 15.0f), 0, 15));
	const uint8 SnapNib = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt((ForcePos / 8.0f) * 15.0f), 0, 15));
	FMemory::Memset(Out, 0, 10);
	Out[0] = 0x22;
	Out[1] = static_cast<uint8>((1 << StartPosition) | 0x02);
	Out[2] = static_cast<uint8>(EndPosition == 8 ? 0x01 : 0x00);
	Out[3] = static_cast<uint8>(((ResistanceNib & 0x0F) << 4) | (SnapNib & 0x0F));
	return true;
}

static bool ComposeGallopBytes(uint8 Out[10], int32 StartPosition, int32 EndPosition, int32 FirstFoot, int32 SecondFoot, int32 Frequency)
{
	// Validate positions following DualSenseLibrary::SetGalloping expectations
	if (StartPosition < 0 || StartPosition > 8)
	{
		return false;
	}
	if (EndPosition < 1 || EndPosition > 9)
	{
		return false;
	}
	if (EndPosition <= StartPosition)
	{
		return false;
	}
	if (FirstFoot < 0 || FirstFoot > 8)
	{
		return false;
	}
	if (SecondFoot < 1 || SecondFoot > 9)
	{
		return false;
	}
	if (SecondFoot <= FirstFoot)
	{
		return false;
	}
	if (Frequency < 0 || Frequency > 255)
	{
		return false;
	}

	const uint8 FirstFootNib = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt((FirstFoot / 8.0f) * 15.0f), 1, 15));
	const uint8 SecondFootNib = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt((SecondFoot / 8.0f) * 15.0f), 1, 15));

	int32 KeepEffect = 0; // 0 none, 1 keep at 8, 2 keep at 9
	if (EndPosition >= 8)
	{
		KeepEffect = 1;
	}
	if (EndPosition >= 9)
	{
		KeepEffect = 2;
	}

	FMemory::Memset(Out, 0, 10);
	Out[0] = 0x23;                                                          // Galloping mode
	Out[1] = static_cast<uint8>((1 << StartPosition) | (1 << EndPosition)); // active zones bitmask
	Out[2] = static_cast<uint8>(KeepEffect);
	Out[3] = static_cast<uint8>(((FirstFootNib & 0x0F) << 4) | (SecondFootNib & 0x0F));
	Out[4] = static_cast<uint8>(Frequency);
	return true;
}

void FCommandHelpers::HandleBowTrigR(const TArray<FString>& Args)
{
	FInputDeviceId DeviceId;
	if (!ParseDeviceId(Args, DeviceId))
	{
		return;
	}
	ISonyGamepad* Gamepad = GetGamepad(DeviceId.GetId());
	if (!Gamepad)
	{
		return;
	}
	FDeviceContext* Ctx = Gamepad->GetMutableDeviceContext();
	if (!Ctx || !Ctx->IsConnected)
	{
		UE_LOG(LogDualSense, Warning, TEXT("Device not ready/connected"));
		return;
	}
	if (Args.Num() < 5)
	{
		UE_LOG(LogDualSense, Warning, TEXT("Usage: ds.BowR <DeviceId> <Start 0-7> <End 0-8> <ResistancePos 0-8> <ForcePos 0-8>"));
		return;
	}
	int32 Start = FCString::Atoi(*Args[1]);
	int32 End = FCString::Atoi(*Args[2]);
	int32 Resistance = FCString::Atoi(*Args[3]);
	int32 Force = FCString::Atoi(*Args[4]);
	uint8 Bytes[10] = {0};
	if (!ComposeBowBytes(Bytes, Start, End, Resistance, Force))
	{
		UE_LOG(LogDualSense, Warning, TEXT("Invalid parameter range."));
		return;
	}
	FMemory::Memcpy(Ctx->OverrideTriggerRight, Bytes, 10);
	Ctx->bOverrideTriggerBytes = true;
	UE_LOG(LogDualSense, Log, TEXT("Right trigger set to Bow effect: [%02X %02X %02X %02X]"), Bytes[0], Bytes[1], Bytes[2], Bytes[3]);
	FGamepadOutput::OutputDualSense(Ctx);
}

void FCommandHelpers::HandleBowTrigL(const TArray<FString>& Args)
{
	FInputDeviceId DeviceId;
	if (!ParseDeviceId(Args, DeviceId))
	{
		return;
	}
	ISonyGamepad* Gamepad = GetGamepad(DeviceId.GetId());
	if (!Gamepad)
	{
		return;
	}
	FDeviceContext* Ctx = Gamepad->GetMutableDeviceContext();
	if (!Ctx || !Ctx->IsConnected)
	{
		UE_LOG(LogDualSense, Warning, TEXT("Device not ready/connected"));
		return;
	}
	if (Args.Num() < 5)
	{
		UE_LOG(LogDualSense, Warning, TEXT("Usage: ds.BowL <DeviceId> <Start 0-7> <End 0-8> <ResistancePos 0-8> <ForcePos 0-8>"));
		return;
	}
	int32 Start = FCString::Atoi(*Args[1]);
	int32 End = FCString::Atoi(*Args[2]);
	int32 Resistance = FCString::Atoi(*Args[3]);
	int32 Force = FCString::Atoi(*Args[4]);
	uint8 Bytes[10] = {0};
	if (!ComposeBowBytes(Bytes, Start, End, Resistance, Force))
	{
		UE_LOG(LogDualSense, Warning, TEXT("Invalid parameter range."));
		return;
	}
	FMemory::Memcpy(Ctx->OverrideTriggerLeft, Bytes, 10);
	Ctx->bOverrideTriggerBytes = true;
	UE_LOG(LogDualSense, Log, TEXT("Left trigger set to Bow effect: [%02X %02X %02X %02X]"), Bytes[0], Bytes[1], Bytes[2], Bytes[3]);
	FGamepadOutput::OutputDualSense(Ctx);
}

void FCommandHelpers::HandleGallopTrigR(const TArray<FString>& Args)
{
	FInputDeviceId DeviceId;
	if (!ParseDeviceId(Args, DeviceId))
	{
		return;
	}
	ISonyGamepad* Gamepad = GetGamepad(DeviceId.GetId());
	if (!Gamepad)
	{
		return;
	}
	FDeviceContext* Ctx = Gamepad->GetMutableDeviceContext();
	if (!Ctx || !Ctx->IsConnected)
	{
		UE_LOG(LogDualSense, Warning, TEXT("Device not ready/connected"));
		return;
	}
	if (Args.Num() < 6)
	{
		UE_LOG(LogDualSense, Warning, TEXT("Usage: ds.GallopR <DeviceId> <Start 0-8> <End 1-9> <FirstFoot 0-8> <SecondFoot 1-9> <Freq 0-255>"));
		return;
	}
	int32 Start = FCString::Atoi(*Args[1]);
	int32 End = FCString::Atoi(*Args[2]);
	int32 FirstFoot = FCString::Atoi(*Args[3]);
	int32 SecondFoot = FCString::Atoi(*Args[4]);
	int32 Freq = FCString::Atoi(*Args[5]);
	uint8 Bytes[10] = {0};
	if (!ComposeGallopBytes(Bytes, Start, End, FirstFoot, SecondFoot, Freq))
	{
		UE_LOG(LogDualSense, Warning, TEXT("Invalid parameter range."));
		return;
	}
	FMemory::Memcpy(Ctx->OverrideTriggerRight, Bytes, 10);
	Ctx->bOverrideTriggerBytes = true;
	UE_LOG(LogDualSense, Log, TEXT("Right trigger set to Gallop effect: [%02X %02X %02X %02X %02X]"), Bytes[0], Bytes[1], Bytes[2], Bytes[3], Bytes[4]);
	FGamepadOutput::OutputDualSense(Ctx);
}

void FCommandHelpers::HandleGallopTrigL(const TArray<FString>& Args)
{
	FInputDeviceId DeviceId;
	if (!ParseDeviceId(Args, DeviceId))
	{
		return;
	}
	ISonyGamepad* Gamepad = GetGamepad(DeviceId.GetId());
	if (!Gamepad)
	{
		return;
	}
	FDeviceContext* Ctx = Gamepad->GetMutableDeviceContext();
	if (!Ctx || !Ctx->IsConnected)
	{
		UE_LOG(LogDualSense, Warning, TEXT("Device not ready/connected"));
		return;
	}
	if (Args.Num() < 6)
	{
		UE_LOG(LogDualSense, Warning, TEXT("Usage: ds.GallopL <DeviceId> <Start 0-8> <End 1-9> <FirstFoot 0-8> <SecondFoot 1-9> <Freq 0-255>"));
		return;
	}
	int32 Start = FCString::Atoi(*Args[1]);
	int32 End = FCString::Atoi(*Args[2]);
	int32 FirstFoot = FCString::Atoi(*Args[3]);
	int32 SecondFoot = FCString::Atoi(*Args[4]);
	int32 Freq = FCString::Atoi(*Args[5]);
	uint8 Bytes[10] = {0};
	if (!ComposeGallopBytes(Bytes, Start, End, FirstFoot, SecondFoot, Freq))
	{
		UE_LOG(LogDualSense, Warning, TEXT("Invalid parameter range."));
		return;
	}
	FMemory::Memcpy(Ctx->OverrideTriggerLeft, Bytes, 10);
	Ctx->bOverrideTriggerBytes = true;
	UE_LOG(LogDualSense, Log, TEXT("Left trigger set to Gallop effect: [%02X %02X %02X %02X %02X]"), Bytes[0], Bytes[1], Bytes[2], Bytes[3], Bytes[4]);
	FGamepadOutput::OutputDualSense(Ctx);
}
