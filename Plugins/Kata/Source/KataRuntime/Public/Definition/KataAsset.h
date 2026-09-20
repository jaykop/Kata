#pragma once

#include "CoreMinimal.h"
#include "Definition/KataDefinition.h"
#include "KataAsset.generated.h"

class AActor;

/** 프리뷰 월드에 표시할 거리·높이 측정 도형. */
UENUM(BlueprintType)
enum class EKataPreviewDebugShape : uint8
{
    Grid,
    Sphere
};

/**
 * Content Browser에 저장하는 Kata 원본 에셋.
 * 부모 에셋의 현재 값에 명시적인 변경분만 합치며, 실행 상태는 UKataInstance가 소유한다.
 */
UCLASS(BlueprintType, NotBlueprintable, meta = (DisplayName = "Kata"))
class KATARUNTIME_API UKataAsset : public UKataDefinition
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Inheritance")
    TObjectPtr<UKataAsset> ParentKata;

    /** 자식에서 수정한 설정 경로. 구조체는 점으로 구분하고 배열과 조건 객체는 전체 값으로 취급한다. */
    UPROPERTY(VisibleAnywhere, Category = "Kata|Inheritance")
    TArray<FName> OverriddenSettings;

#if WITH_EDITORONLY_DATA
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

    /** bForEditing이면 비활성·미완성 태스크도 반환한다. 실행에는 기본값 false를 사용한다. */
    UKataResolvedDefinition* Resolve(UObject* Outer, bool bForEditing = false) const;

    /** 부모부터 자식 순서로 수집한다. 순환 관계가 있으면 false를 반환한다. */
    bool CollectAssetChain(TArray<const UKataAsset*>& OutChain) const;

    /** 편집 화면에 표시할 상속된 설정 사본을 만든다. 원본 에셋은 변경하지 않는다. */
    UKataAsset* MakeEffectiveSettings(UObject* Outer) const;

#if WITH_EDITOR
    virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& Event) override;
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
