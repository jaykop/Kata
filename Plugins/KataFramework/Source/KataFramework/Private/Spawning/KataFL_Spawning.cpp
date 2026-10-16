#include "Spawning/KataFL_Spawning.h"

#include "AIController.h"
#include "Character/KataCharacter.h"
#include "Character/KataCharacterSpawnOwnership.h"
#include "Components/CapsuleComponent.h"
#include "UObject/StrongObjectPtr.h"

float KataFL::GetCharacterClassHalfHeight(const UClass* CharacterClass)
{
    // Blueprint에서 바꾼 캡슐 크기는 생성된 클래스의 기본 객체에서 인스턴스 없이 읽는다.
    // 기본 객체는 등록되지 않아 월드 Transform이 단위 행렬이므로 GetScaledCapsuleHalfHeight를 쓸 수 없다.
    // 대신 그 함수와 같이 스케일 적용 전 절반 높이에 루트 캡슐의 상대 Z 스케일을 곱한다.
    const ACharacter* CharacterDefaults = CharacterClass != nullptr ? Cast<ACharacter>(CharacterClass->GetDefaultObject()) : nullptr;
    const UCapsuleComponent* Capsule = CharacterDefaults != nullptr ? CharacterDefaults->GetCapsuleComponent() : nullptr;
    return Capsule != nullptr ? Capsule->GetUnscaledCapsuleHalfHeight() * static_cast<float>(Capsule->GetRelativeScale3D().Z) : 0.f;
}

bool KataFL::DestroySpawnedCharacter(FKataCharacterSpawnOwnership& Ownership)
{
    Ownership.bDespawnRequested = true;
    AKataCharacter* Character = Ownership.Character.Get();
    if (Character == nullptr || Character->IsActorBeingDestroyed())
    {
        return true;
    }
    const TStrongObjectPtr<AKataCharacter> KeepAlive(Character);
    return Character->Destroy();
}

bool KataFL::DestroySpawnOwnedController(const FKataCharacterSpawnOwnership& Ownership, int32 ControllerIndex)
{
    AController* Controller = Ownership.OwnedControllers[ControllerIndex].Get();
    if (Controller == nullptr || Controller->IsActorBeingDestroyed() || !Controller->IsA<AAIController>())
    {
        return true;
    }
    const TStrongObjectPtr<AController> KeepAlive(Controller);
    if (Controller->GetPawn() != nullptr && Controller->GetPawn() != Ownership.Character.Get())
    {
        return true;
    }
    if (Controller->GetPawn() != nullptr)
    {
        Controller->UnPossess();
    }
    // UnPossess 안의 확장 코드가 다른 Pawn에 재사용했으면 제거하지 않는다.
    if (!IsValid(Controller) || Controller->IsActorBeingDestroyed() || Controller->GetPawn() != nullptr)
    {
        return true;
    }
    return Controller->Destroy();
}
