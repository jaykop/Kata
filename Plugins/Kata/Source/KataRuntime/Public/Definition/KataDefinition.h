#pragma once

#include "CoreMinimal.h"
#include "KataRuntimeTypes.h"
#include "Templates/SubclassOf.h"
#include "UObject/Object.h"
#include "KataDefinition.generated.h"

class UKataCondition;
class UKataResolvedDefinition;
class UKataTask;

/** 공통 Kata 설정과 기존 Blueprint 정의의 호환 계층. 새 콘텐츠는 UKataAsset으로 작성한다. */
UCLASS(Abstract, Blueprintable, BlueprintType)
class KATARUNTIME_API UKataDefinition : public UObject
{
    GENERATED_BODY()

public:
    /** 이 Kata 자체의 분류 태그. 실행 중 주체 ASC에 자동으로 부여하지 않는다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Identity")
    FGameplayTagContainer KataTags;

    /** 실행 주체에 모두 있어야 시작할 수 있는 태그. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Activation")
    FGameplayTagContainer ActivationRequiredTags;

    /** 실행 주체에 하나라도 있으면 시작할 수 없는 태그. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Activation")
    FGameplayTagContainer ActivationBlockedTags;

    /** 실행 중 주체에 부여하고 종료 시 자신의 기여분만 회수할 태그. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Activation")
    FGameplayTagContainer ActiveGrantedTags;

    /** 추가 공용 조건. 비워 두면 허용하고, 설정하면 최종 Pass만 허용한다. */
    UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Kata|Activation")
    TObjectPtr<UKataCondition> StartCondition;

    /** 실행 중 다른 Kata와 다른 Ability를 막는 정책. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Blocking")
    FKataBlockingPolicy BlockingPolicy;

    /** 쿨다운 설정. 진행 상태는 GAS가 보관한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Cooldown")
    FKataCooldownPolicy CooldownPolicy;

    /** 인스턴스 안에서 타임라인을 반복하는 정책. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Loop")
    FKataLoopPolicy LoopPolicy;

    /**
     * 이 클래스가 선언하는 타임라인 항목.
     * 상속으로 보이는 부모 항목은 편집하지 않고 TaskOverrides로 변경한다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Timeline")
    TArray<FKataTimelineEntry> TimelineTasks;

    /** 상속받은 항목에 대한 이 클래스의 변경분. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Timeline")
    TArray<FKataTaskOverride> TaskOverrides;

    /**
     * 기존 Blueprint 정의 클래스를 해석한다. 새 에셋은 UKataAsset::Resolve를 사용한다.
     *
     * @param DefinitionClass 해석할 가장 파생된 정의 클래스.
     * @param Outer           해석 결과와 태스크 사본을 소유할 객체.
     * @param bForEditing     true이면 가져오기용으로 미완성·비활성 태스크도 포함한다.
     * @return 해석 결과. 실패해도 진단을 담은 객체를 반환한다. 클래스가 유효하지 않으면 null.
     */
    static UKataResolvedDefinition* ResolveDefinition(TSubclassOf<UKataDefinition> DefinitionClass, UObject* Outer, bool bForEditing = false);

    /** 부모 클래스 체인에서 상속받는 타임라인 항목을 찾는다. 자기 클래스의 선언은 제외한다. */
    const UKataTask* FindInheritedTaskTemplate(const FKataTaskId& TaskId) const;

#if WITH_EDITOR
    virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

protected:
    /** 부모부터 자식까지 정렬한 정의를 병합한다. 에셋과 기존 클래스 경로가 같은 실행 규칙을 사용한다. */
    static UKataResolvedDefinition* ResolveChain(const TArray<const UKataDefinition*>& Chain,
        const UKataDefinition* EffectiveSettings, UObject* Outer, bool bAssetChain, bool bForEditing = false);

#if WITH_EDITOR
    /** 새로 추가된 항목에 이 클래스를 선언자로 기록하고 태스크 ID를 발급한다. */
    void StampDeclaringClasses();

    /**
     * 오버라이드 값에서 바뀐 프로퍼티를 OverriddenProperties에 자동으로 기록한다.
     * 값이 부모와 같아졌다는 이유만으로 항목을 제거하지는 않는다.
     */
    void TrackOverriddenProperty(FPropertyChangedChainEvent& PropertyChangedEvent);

    /** 대상 항목이 정해졌는데 오버라이드 사본이 없거나 타입이 다르면 상속 원본으로 다시 만든다. */
    void SyncOverrideTemplate(FKataTaskOverride& Override);
#endif
};
