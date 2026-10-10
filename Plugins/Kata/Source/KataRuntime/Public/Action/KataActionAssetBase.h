#pragma once

#include "CoreMinimal.h"
#include "KataRuntimeTypes.h"
#include "UObject/Object.h"
#include "KataActionAssetBase.generated.h"

class AActor;
class UKataCommand;
class UKataCondition;
class UKataPreviewSetup;
class UKataResolvedAction;
class UKataTask;
class UKataActionTemplate;

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
 * UKataAction과 UKataActionTemplate이 공유하는 액션 에셋 기반.
 *
 * 태그·조건·명령·타임라인·프리뷰 설정과 해석·검증 로직을 소유한다. 상속은 UKataAction이 부모 Template 하나만
 * 참조하는 1단계로 제한하므로 해석 체인은 [Template, Action] 또는 [자기 자신] 중 하나다.
 * 필드 이름은 기존 UKataAction 에셋의 직렬화 이름과 같아야 하므로 바꾸지 않는다.
 * NotBlueprintable 지정자는 UHT에서 BlueprintType 메타데이터를 지우므로 블루프린트 상속 차단은 IsBlueprintBase 메타데이터로 지정한다.
 */
UCLASS(Abstract, BlueprintType, meta = (IsBlueprintBase = "false"))
class KATARUNTIME_API UKataActionAssetBase : public UObject
{
    GENERATED_BODY()

public:
    /** 이 액션 자체의 분류 태그. 실행 중 주체 ASC에 자동으로 부여하지 않는다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Tag")
    FGameplayTagContainer KataTags;

    /** 실행 주체에 모두 있어야 시작할 수 있는 태그. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Tag")
    FGameplayTagContainer ActivationRequiredTags;

    /** 실행 주체에 하나라도 있으면 시작할 수 없는 태그. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Tag")
    FGameplayTagContainer ActivationBlockedTags;

    /** 실행 중 주체에 부여하고 종료 시 자신의 기여분만 회수할 태그. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Tag")
    FGameplayTagContainer ActiveGrantedTags;

    /** 실행 중 다른 액션과 다른 Ability를 막는 정책. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Tag")
    FKataBlockingPolicy BlockingPolicy;

    /** 추가 공용 조건. 비워 두면 허용하고, 설정하면 최종 Pass만 허용한다. */
    UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Kata|Condition")
    TObjectPtr<UKataCondition> StartCondition;

    /** 쿨다운 설정. 진행 상태는 GAS가 보관한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Cooldown")
    FKataCooldownPolicy CooldownPolicy;

    /** 인스턴스 안에서 타임라인을 반복하는 정책. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Loop")
    FKataLoopPolicy LoopPolicy;

    /**
     * 시작 조건을 통과해 액션이 시작될 때 타임라인보다 먼저 선언 순서대로 한 번씩 실행하는 명령.
     * 반복 액션이어도 첫 시작에서만 실행한다. 이 목록의 명령만 이번 실행의 대상을 바꿀 수 있다.
     */
    UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Kata|Command")
    TArray<TObjectPtr<UKataCommand>> PreCommands;

    /**
     * 액션이 종료될 때 타임라인 태스크를 정리한 뒤 선언 순서대로 한 번씩 실행하는 명령.
     * 항목마다 실행할 종료 사유를 고를 수 있다. 시작하지 못하고 끝난 액션에서는 실행하지 않는다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Command")
    TArray<FKataPostCommandEntry> PostCommands;

    /**
     * 이 에셋이 직접 선언하는 타임라인 항목.
     * 자식 UKataAction은 부모 Template의 항목을 편집하지 않고 UKataAction::TaskOverrides로 변경한다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Timeline")
    TArray<FKataTimelineEntry> TimelineTasks;

#if WITH_EDITORONLY_DATA
    /** 이 에셋에서만 사용하는 타임라인 그룹 배치. 부모 에셋으로부터 상속하지 않는다. */
    UPROPERTY(EditAnywhere, Category = "Timeline Groups")
    TArray<FKataTimelineGroup> TimelineGroups;

    /**
     * 타임라인 최상위 행의 표시 순서. 항목은 그룹의 GroupId이거나 그룹에 속하지 않은 태스크의 TaskId 값이며,
     * 그룹과 태스크를 섞어 둘 수 있다. 그룹 안의 순서는 FKataTimelineGroup::TaskIds가 정한다.
     * 실행 순서에는 영향을 주지 않고 부모 에셋으로부터 상속하지 않는다.
     * 목록에 없는 그룹은 소속 태스크가 목록에 있던 자리에, 그것도 없으면 뒤에 표시하고, 목록에 없는 태스크는 선언 순서대로 맨 뒤에 표시한다.
     */
    UPROPERTY()
    TArray<FGuid> TimelineTopLevelOrder;

    /** 프리뷰에서 생성할 캐릭터 클래스. 실제 게임 월드의 액터는 사용하지 않는다. */
    UPROPERTY(EditAnywhere, Category = "Preview")
    TSubclassOf<AActor> PreviewActorClass;

    UPROPERTY(EditAnywhere, Category = "Preview")
    TSubclassOf<AActor> PreviewTargetClass;

    /**
     * 프리뷰 액터를 스폰한 직후 적용할 준비 설정. 위성 플러그인이 제공하는 설정(예: 장비 장착)을 골라 추가한다.
     * 프리뷰 월드에만 적용하며 부모 에셋으로부터 상속하지 않는다. 새 자식 에셋을 만들 때 부모의 설정을 복사한다.
     */
    UPROPERTY(EditAnywhere, Instanced, Category = "Preview")
    TArray<TObjectPtr<UKataPreviewSetup>> PreviewSetups;

    /** 기본 Yaw 180은 -X에 놓인 Target을 마주 보게 한다. */
    UPROPERTY(EditAnywhere, Category = "Preview")
    FTransform PreviewActorTransform = FTransform(FRotator(0.0, 180.0, 0.0), FVector(0.0, 0.0, 100.0));

    UPROPERTY(EditAnywhere, Category = "Preview")
    FTransform PreviewTargetTransform = FTransform(FRotator::ZeroRotator, FVector(-500.0, 0.0, 100.0));

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
    FVector PreviewEnvironmentSize = FVector(10000.0, 10000.0, 1000.0);

    /** -X 쪽 앞 벽을 표시한다. 바닥은 항상 표시한다. */
    UPROPERTY(EditAnywhere, Category = "Preview|Environment", meta = (DisplayName = "Show Front Wall"))
    bool bPreviewShowFrontWall = false;

    /** +Y 쪽 옆 벽을 표시한다. */
    UPROPERTY(EditAnywhere, Category = "Preview|Environment", meta = (DisplayName = "Show Side Wall"))
    bool bPreviewShowSideWall = false;

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

    /** bForEditing이면 비활성·미완성 태스크도 반환한다. 실행에는 기본값 false를 사용한다. */
    UKataResolvedAction* Resolve(UObject* Outer, bool bForEditing = false) const;

    /** 이 에셋이 UKataAction이면 부모 Template을, Template이거나 부모가 없으면 nullptr을 반환한다. */
    const UKataActionTemplate* GetParentTemplate() const;

    /** 부모 Template, 자신 순서로 수집한다. 부모가 없으면 자신만 담는다. */
    void CollectActionChain(TArray<const UKataActionAssetBase*>& OutChain) const;

    /** 편집 화면에 표시할 상속된 설정 사본을 만든다. 사본은 이 에셋과 같은 클래스이며 원본 에셋은 변경하지 않는다. */
    UKataActionAssetBase* MakeEffectiveSettings(UObject* Outer) const;

#if WITH_EDITOR
    virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& Event) override;
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

private:
    /** 부모부터 자식까지 정렬한 에셋 체인을 병합한다. */
    static UKataResolvedAction* ResolveChain(const TArray<const UKataActionAssetBase*>& Chain,
        const UKataActionAssetBase* EffectiveSettings, UObject* Outer, bool bForEditing);
};
