#pragma once

#include "CoreMinimal.h"
#include "Action/KataActionAssetBase.h"
#include "KataAction.generated.h"

class UKataActionTemplate;

/**
 * Content Browser에 저장하는 재생 가능한 액션 하나의 원본 에셋.
 *
 * 부모 Template의 현재 값에 명시적인 변경분만 합치며, 실행 상태는 UKataActionInstance가 소유한다.
 * 상속은 ParentAction으로 UKataActionTemplate 하나만 참조하는 1단계다. UKataAction은 다른 액션의 부모가 될 수 없다.
 * NotBlueprintable 지정자는 UHT에서 BlueprintType 메타데이터를 지우므로 블루프린트 상속 차단은 IsBlueprintBase 메타데이터로 지정한다.
 */
UCLASS(BlueprintType, meta = (IsBlueprintBase = "false", DisplayName = "Kata Action"))
class KATARUNTIME_API UKataAction : public UKataActionAssetBase
{
    GENERATED_BODY()

public:
    /** 부모 Template에서 상속받은 항목에 대한 이 에셋의 변경분. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Timeline")
    TArray<FKataTaskOverride> TaskOverrides;

    /**
     * 설정을 물려받을 부모 Template. 비워 두면 단독 액션으로 해석한다.
     * 프로퍼티 이름은 기존 에셋의 직렬화 이름이므로 유지하고 표시 이름만 바꾼다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Inheritance", meta = (DisplayName = "Parent Template"))
    TObjectPtr<UKataActionTemplate> ParentAction;

    /** 자식에서 수정한 설정 경로. 구조체는 점으로 구분하고 배열과 조건 객체는 전체 값으로 취급한다. */
    UPROPERTY(VisibleAnywhere, Category = "Kata|Inheritance")
    TArray<FName> OverriddenSettings;

    virtual void PostLoad() override;

#if WITH_EDITOR
    virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& Event) override;
#endif
};
