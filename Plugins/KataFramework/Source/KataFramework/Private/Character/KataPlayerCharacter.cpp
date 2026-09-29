#include "Character/KataPlayerCharacter.h"

#include "Character/KataCharacterRow.h"
#include "Input/KataInputConfig.h"
#include "Input/KataInputHandlerComponent.h"
#include "KataGraph.h"
#include "StructUtils/InstancedStruct.h"
#include "Targeting/KataPlayerTargetingComponent.h"

AKataPlayerCharacter::AKataPlayerCharacter(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer.SetDefaultSubobjectClass<UKataPlayerTargetingComponent>(AKataCharacter::TargetingComponentName))
{
    InputHandlerComponent = CreateDefaultSubobject<UKataInputHandlerComponent>(TEXT("KataInputHandlerComponent"));
}

UKataPlayerTargetingComponent* AKataPlayerCharacter::GetPlayerTargetingComponent() const
{
    // 파생 클래스가 서브오브젝트 타입을 PC용이 아닌 타입으로 다시 바꿀 수 있으므로 Cast로 확인한다.
    return Cast<UKataPlayerTargetingComponent>(GetTargetingComponent());
}

void AKataPlayerCharacter::ApplyCharacterRow(const FInstancedStruct& RowData)
{
    Super::ApplyCharacterRow(RowData);

    const FKataPlayerCharacterRow* Row = RowData.GetPtr<FKataPlayerCharacterRow>();
    if (Row == nullptr)
    {
        return;
    }

    if (UKataInputConfig* InputConfig = Row->InputConfig.Get())
    {
        InputHandlerComponent->SetInputConfig(InputConfig);
    }
    if (UKataGraph* Graph = Row->Graph.Get())
    {
        InputHandlerComponent->SetGraph(Graph);
    }
}

void AKataPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    InputHandlerComponent->SetupPlayerInput(PlayerInputComponent);
}
