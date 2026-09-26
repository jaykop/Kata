#include "Character/KataPlayerCharacter.h"

#include "Targeting/KataPlayerTargetingComponent.h"

AKataPlayerCharacter::AKataPlayerCharacter(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer.SetDefaultSubobjectClass<UKataPlayerTargetingComponent>(AKataCharacter::TargetingComponentName))
{
}

UKataPlayerTargetingComponent* AKataPlayerCharacter::GetPlayerTargetingComponent() const
{
    // 파생 클래스가 서브오브젝트 타입을 PC용이 아닌 타입으로 다시 바꿀 수 있으므로 Cast로 확인한다.
    return Cast<UKataPlayerTargetingComponent>(GetTargetingComponent());
}
