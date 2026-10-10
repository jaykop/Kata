#include "Implementations/Adapters/DeviceRegistryPolicy.h"
#include "GCore/Interfaces/ISonyGamepad.h"
#include "GCore/Types/ECoreGamepad.h"
#include "GameFramework/InputSettings.h"
#include "GenericPlatform/InputDeviceRegistry.h"
#include "Implementations/Managers/SonyGamepadReader.h"

namespace
{
    /** FInputDeviceDescriptor와 FHardwareDeviceIdentifier의 입력 클래스 이름. 이 플러그인의 모든 장치가 공유한다. */
    const FName SonyInputDeviceName(TEXT("WindowsDualsense"));

    FName GetHardwareDeviceIdentifier(const EDSDeviceType DeviceType)
    {
        switch (DeviceType)
        {
            case EDSDeviceType::DualSenseEdge:
                return FName(TEXT("DualSenseEdge"));
            case EDSDeviceType::DualShock4:
                return FName(TEXT("DualShock4"));
            default:
                return FName(TEXT("DualSense"));
        }
    }

    /** 플러그인이 실제로 제공하는 기능만 표시한다. DualShock 4에는 적응형 트리거와 오디오 햅틱이 없다. */
    EHardwareDeviceSupportedFeatures::Type GetSupportedFeatures(const EDSDeviceType DeviceType)
    {
        using namespace EHardwareDeviceSupportedFeatures;
        Type Features = Gamepad | Touch | MotionTracking | Acceleration | Lights | ForceFeedback;
        if (DeviceType != EDSDeviceType::DualShock4)
        {
            Features |= TriggerHaptics | AudioBasedVibrations;
        }
        return Features;
    }
}

FDeviceRegistryPolicy::FDeviceRegistryPolicy() = default;

void FDeviceRegistryPolicy::RegisterDeviceDescriptor(EngineIdType GamepadId, const std::shared_ptr<ISonyGamepad>& Gamepad)
{
    check(IsInGameThread());
    if (!Gamepad || !GamepadId.IsValid())
    {
        return;
    }

    const EDSDeviceType DeviceType = Gamepad->GetDeviceType();
    const FName HardwareDeviceIdentifier = GetHardwareDeviceIdentifier(DeviceType);

    // UInputDeviceSubsystem은 이 목록에서 하드웨어 식별자를 찾아 장치 유형을 정한다. 목록에 없으면 유형이 Unspecified가 된다.
    // XInput은 플러그인 Config로 목록에 넣지만, 이 플러그인은 장치를 처음 감지할 때(엔진 초기화 이후) 런타임에 추가한다.
    if (UInputPlatformSettings* PlatformSettings = UInputPlatformSettings::Get())
    {
        if (PlatformSettings->GetHardwareDeviceForClassName(HardwareDeviceIdentifier) == nullptr)
        {
            PlatformSettings->AddHardwareDeviceIdentifier(FHardwareDeviceIdentifier(
                SonyInputDeviceName, HardwareDeviceIdentifier, EHardwareDevicePrimaryType::Gamepad, GetSupportedFeatures(DeviceType)));
        }
    }

    // 같은 DeviceId로 재연결할 때 다시 호출해도 내용이 같으면 변경 이벤트가 나가지 않는다.
    FInputDeviceDescriptor Descriptor;
    Descriptor.HardwareDeviceHandle = GamepadId;
    Descriptor.InputDeviceName = SonyInputDeviceName;
    Descriptor.HardwareDeviceIdentifier = HardwareDeviceIdentifier;
    FInputDeviceRegistry::RegisterDevice(Descriptor);
}

FDeviceRegistryPolicy::~FDeviceRegistryPolicy()
{
    // 각 리더의 소멸자가 Shutdown을 호출해 스레드가 끝날 때까지 기다린다.
    Readers.Empty();
}

void FDeviceRegistryPolicy::StartReader(EngineIdType GamepadId, const std::shared_ptr<ISonyGamepad>& Gamepad)
{
    check(IsInGameThread());
    if (!Gamepad)
    {
        return;
    }

    StopReader(GamepadId);
    Readers.Add(GamepadId, MakeUnique<FSonyGamepadReader>(Gamepad, GamepadId.GetId()));
}

void FDeviceRegistryPolicy::StopReader(EngineIdType GamepadId)
{
    check(IsInGameThread());

    if (TUniquePtr<FSonyGamepadReader>* Reader = Readers.Find(GamepadId))
    {
        if (Reader->IsValid())
        {
            (*Reader)->Shutdown();
        }
        Readers.Remove(GamepadId);
    }
}
