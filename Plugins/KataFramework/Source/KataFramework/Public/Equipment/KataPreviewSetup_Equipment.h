#pragma once

#include "Action/KataPreviewSetup.h"
#include "CoreMinimal.h"
#include "Equipment/KataEquipmentRow.h"
#include "KataPreviewSetup_Equipment.generated.h"

class UKataEquipmentSetup;

/**
 * 액션 편집기의 프리뷰 액터에 장비를 장착한 상태로 보여 주는 준비 설정. 액션 에셋의 Preview Setups에 추가한다.
 *
 * 프리뷰 액터는 캐릭터 행이 적용되지 않으므로 장비 설정과 장착할 장비를 여기서 지정한다.
 * 프리뷰 액터에 장착 컴포넌트가 없으면 임시로 붙인다. 장착은 동기로 해서 액션 재생 첫 프레임부터 무기 판정에 쓸 수 있다.
 */
UCLASS(meta = (DisplayName = "Equipment"))
class KATAFRAMEWORK_API UKataPreviewSetup_Equipment : public UKataPreviewSetup
{
    GENERATED_BODY()

public:
    /** 프리뷰 액터의 장착 컴포넌트에 지정할 장비 설정. 비워 두면 컴포넌트의 Blueprint 기본값을 쓴다. */
    UPROPERTY(EditAnywhere, Category = "Equipment")
    TObjectPtr<UKataEquipmentSetup> EquipmentSetup;

    /** 프리뷰에서 장착할 장비와 슬롯. */
    UPROPERTY(EditAnywhere, Category = "Equipment")
    TArray<FKataStartingEquipment> Equipment;

    virtual void ApplyToPreview(AActor* PreviewActor) const override;
};
