#include "Character/KataAICharacter.h"

#include "Character/KataCharacterRow.h"
#include "Controller/KataAIController.h"
#include "Data/KataAIData.h"
#include "StructUtils/InstancedStruct.h"
#include "Targeting/KataAITargetingComponent.h"

AKataAICharacter::AKataAICharacter(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer.SetDefaultSubobjectClass<UKataAITargetingComponent>(AKataCharacter::TargetingComponentName))
{
    AIControllerClass = AKataAIController::StaticClass();
}

void AKataAICharacter::ApplyCharacterRow(const FInstancedStruct& RowData)
{
    Super::ApplyCharacterRow(RowData);
    const FKataNPCCharacterRow* Row = RowData.GetPtr<FKataNPCCharacterRow>();
    if (Row == nullptr)
    {
        return;
    }
    if (!Row->AIControllerClass.IsNull())
    {
        AIControllerClass = Row->AIControllerClass.Get();
    }
    // 빈 행 설정도 반영해 이전 Pawn 설정이 남지 않게 한다.
    AIData = Row->AIData.Get();
}

void AKataAICharacter::BeginPlay()
{
    Super::BeginPlay();
    // BeginPlay 콜백 중에는 HasActorBegunPlay가 아직 false이므로 준비 완료를 명시한다.
    bAIReady = true;
    if (AKataAIController* AIController = Cast<AKataAIController>(GetController()))
    {
        AIController->TryStartKataAI();
    }
}

void AKataAICharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    bAIReady = false;
    // Pawn을 참조하는 StateTree를 컴포넌트 파괴보다 먼저 정리한다.
    if (AKataAIController* AIController = Cast<AKataAIController>(GetController()))
    {
        AIController->UnPossess();
    }
    Super::EndPlay(EndPlayReason);
}
