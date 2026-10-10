#include "Implementations/Adapters/DeviceRegistryPolicy.h"
#include "GCore/Interfaces/ISonyGamepad.h"
#include "Implementations/Managers/SonyGamepadReader.h"

FDeviceRegistryPolicy::FDeviceRegistryPolicy() = default;

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
