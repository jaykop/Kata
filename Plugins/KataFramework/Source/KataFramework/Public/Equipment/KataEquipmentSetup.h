#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "KataEquipmentSetup.generated.h"

/**
 * 캐릭터 스켈레톤에 따라 정해지는 장비 부착 설정. 같은 스켈레톤을 쓰는 캐릭터끼리 하나를 공유한다.
 *
 * 캐릭터 행의 Equipment Setup이 지정하며, 행이 비어 있으면 장착 컴포넌트의 Blueprint 기본값을 쓴다.
 * 장비는 손에 독립적이므로 같은 장비라도 슬롯에 따라 여기 정한 소켓으로 붙는다.
 */
UCLASS(BlueprintType)
class KATAFRAMEWORK_API UKataEquipmentSetup : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    /** 슬롯마다 부품을 붙일 소켓 이름. 슬롯이 없거나 소켓이 None이면 부착 대상의 원점에 붙는다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (Categories = "Equipment.Slot"))
    TMap<FGameplayTag, FName> SlotSockets;

    /** 장착할 때 대상 슬롯을 지정하지 않으면 쓰는 슬롯. 장비가 허용하지 않으면 장비의 첫 허용 슬롯을 쓴다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (Categories = "Equipment.Slot"))
    FGameplayTag DefaultSlot;
};
