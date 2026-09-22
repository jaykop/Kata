#pragma once

#include "CoreMinimal.h"
#include "KataRuntimeTypes.h"
#include "UObject/Object.h"
#include "KataAction.generated.h"

class AActor;
class UKataCondition;
class UKataResolvedAction;
class UKataTask;

/** KataAction 에디터에서만 사용하는 타임라인 태스크 그룹. 런타임 실행에는 관여하지 않는다. */
USTRUCT()
struct KATARUNTIME_API FKataTimelineGroup
{
    GENERATED_BODY()

    /** 그룹을 사용자별 접힘 상태와 연결하는 안정적인 ID. */
    UPROPERTY(VisibleAnywhere, Category = "Timeline Group")
    FGuid GroupId;

    /** 타임라인 그룹 헤더에 표시할 제목. */
    UPROPERTY(EditAnywhere, Category = "Timeline Group")
    FText Title;

    /** 타임라인 그룹 헤더에 사용할 편집기 전용 색상. */
    UPROPERTY(EditAnywhere, Category = "Timeline Group", meta = (DisplayName = "Display Color"))
    FLinearColor DisplayColor = FLinearColor(0.12f, 0.42f, 0.58f);

    /** 그룹의 용도를 설명하는 편집기 전용 주석. */
    UPROPERTY(EditAnywhere, Category = "Timeline Group", meta = (DisplayName = "Editor Comment", MultiLine = true))
    FText EditorComment;

    /** 이 그룹에 표시할 태스크. 배열 순서가 그룹 안의 표시 순서다. */
    UPROPERTY(VisibleAnywhere, Category = "Timeline Group")
    TArray<FKataTaskId> TaskIds;
};

/** 프리뷰 월드에 표시할 거리·높이 측정 도형. */
UENUM(BlueprintType)
enum class EKataPreviewDebugShape : uint8
{
    Grid,
    Sphere
};

/**
 * Content Browser에 저장하는 액션 하나의 원본 에셋.
 *
 * 부모 에셋의 현재 값에 명시적인 변경분만 합치며, 실행 상태는 UKataActionInstance가 소유한다.
 * 상속은 클래스 계층이 아니라 ParentAction 객체 참조로 표현한다.
 */
UCLASS(BlueprintType, NotBlueprintable, meta = (DisplayName = "Kata Action"))
class KATARUNTIME_API UKataAction : public UObject
{
    GENERATED_BODY()

public:
    /** 이 액션 자체의 분류 태그. 실행 중 주체 ASC에 자동으로 부여하지 않는다. */
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

    /** 실행 중 다른 액션과 다른 Ability를 막는 정책. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Blocking")
    FKataBlockingPolicy BlockingPolicy;

    /** 쿨다운 설정. 진행 상태는 GAS가 보관한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Cooldown")
    FKataCooldownPolicy CooldownPolicy;

    /** 인스턴스 안에서 타임라인을 반복하는 정책. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Loop")
    FKataLoopPolicy LoopPolicy;

    /**
     * 이 에셋이 직접 선언하는 타임라인 항목.
     * 상속으로 보이는 부모 항목은 편집하지 않고 TaskOverrides로 변경한다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Timeline")
    TArray<FKataTimelineEntry> TimelineTasks;

    /** 상속받은 항목에 대한 이 에셋의 변경분. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Timeline")
    TArray<FKataTaskOverride> TaskOverrides;

    /** 설정을 물려받을 부모 액션. 순환 관계는 해석 단계에서 거절한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Inheritance")
    TObjectPtr<UKataAction> ParentAction;

    /** 자식에서 수정한 설정 경로. 구조체는 점으로 구분하고 배열과 조건 객체는 전체 값으로 취급한다. */
    UPROPERTY(VisibleAnywhere, Category = "Kata|Inheritance")
    TArray<FName> OverriddenSettings;

#if WITH_EDITORONLY_DATA
    /** 이 에셋에서만 사용하는 타임라인 그룹 배치. 부모 에셋으로부터 상속하지 않는다. */
    UPROPERTY(EditAnywhere, Category = "Timeline Groups")
    TArray<FKataTimelineGroup> TimelineGroups;

    /** 프리뷰에서 생성할 캐릭터 클래스. 실제 게임 월드의 액터는 사용하지 않는다. */
    UPROPERTY(EditAnywhere, Category = "Preview")
    TSubclassOf<AActor> PreviewActorClass;

    UPROPERTY(EditAnywhere, Category = "Preview")
    TSubclassOf<AActor> PreviewTargetClass;

    /** 기본 Yaw 180은 -X에 놓인 Target을 마주 보게 한다. */
    UPROPERTY(EditAnywhere, Category = "Preview")
    FTransform PreviewActorTransform = FTransform(FRotator(0.0, 180.0, 0.0), FVector(0.0, 0.0, 100.0));

    UPROPERTY(EditAnywhere, Category = "Preview")
    FTransform PreviewTargetTransform = FTransform(FRotator::ZeroRotator, FVector(-200.0, 0.0, 100.0));

    /** 프리뷰 장면의 Directional Light 방향. 프리뷰 월드에만 적용한다. */
    UPROPERTY(EditAnywhere, Category = "Preview|Lighting")
    FRotator PreviewLightRotation = FRotator(-40.0f, 157.5f, 0.0f);

    /** Directional Light 밝기. */
    UPROPERTY(EditAnywhere, Category = "Preview|Lighting", meta = (ClampMin = "0.0", UIMax = "20.0"))
    float PreviewLightBrightness = UE_PI;

    /** Directional Light 색. */
    UPROPERTY(EditAnywhere, Category = "Preview|Lighting")
    FLinearColor PreviewLightColor = FLinearColor::White;

    /** 프리뷰 뷰포트의 배경색. */
    UPROPERTY(EditAnywhere, Category = "Preview|Environment")
    FLinearColor PreviewBackgroundColor = FLinearColor(0.015f, 0.02f, 0.025f);

    /** 바닥 X/Y 크기와 벽 높이 Z(cm). 바닥과 벽을 함께 조절한다. */
    UPROPERTY(EditAnywhere, Category = "Preview|Environment", meta = (ClampMin = "100.0", UIMin = "100.0"))
    FVector PreviewEnvironmentSize = FVector(2000.0, 2000.0, 1000.0);

    /** 선택한 측정 도형을 표시한다. */
    UPROPERTY(EditAnywhere, Category = "Preview|Debug", meta = (DisplayName = "Show Debug Shape"))
    bool bPreviewShowDebugShape = false;

    /** Grid와 Sphere 중 표시할 측정 도형. */
    UPROPERTY(EditAnywhere, Category = "Preview|Debug")
    EKataPreviewDebugShape PreviewDebugShape = EKataPreviewDebugShape::Grid;

    /** 측정 도형의 선 색. */
    UPROPERTY(EditAnywhere, Category = "Preview|Debug")
    FLinearColor PreviewDebugColor = FLinearColor(0.16f, 0.55f, 0.68f, 0.65f);

    /** 측정 도형의 선 두께. */
    UPROPERTY(EditAnywhere, Category = "Preview|Debug", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "10.0"))
    float PreviewDebugThickness = 2.0f;

    /** 측정 격자와 거리 구 사이의 간격(cm). */
    UPROPERTY(EditAnywhere, Category = "Preview|Debug", meta = (ClampMin = "1.0", UIMin = "10.0", UIMax = "500.0"))
    float PreviewGridCellSize = 100.0f;
#endif

    virtual void PostLoad() override;

    /** bForEditing이면 비활성·미완성 태스크도 반환한다. 실행에는 기본값 false를 사용한다. */
    UKataResolvedAction* Resolve(UObject* Outer, bool bForEditing = false) const;

    /** 부모부터 자식 순서로 수집한다. 순환 관계가 있으면 false를 반환한다. */
    bool CollectActionChain(TArray<const UKataAction*>& OutChain) const;

    /** 편집 화면에 표시할 상속된 설정 사본을 만든다. 원본 에셋은 변경하지 않는다. */
    UKataAction* MakeEffectiveSettings(UObject* Outer) const;

#if WITH_EDITOR
    virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& Event) override;
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

private:
    /** 부모부터 자식까지 정렬한 에셋 체인을 병합한다. */
    static UKataResolvedAction* ResolveChain(const TArray<const UKataAction*>& Chain,
        const UKataAction* EffectiveSettings, UObject* Outer, bool bForEditing);
};
