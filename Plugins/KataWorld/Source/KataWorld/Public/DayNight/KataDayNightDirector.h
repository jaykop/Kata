#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"
#include "KataDayNightDirector.generated.h"

class ADirectionalLight;
class AExponentialHeightFog;
class ASkyAtmosphere;
class ASkyLight;
class UDirectionalLightComponent;
class UExponentialHeightFogComponent;
class UKataDayNightData;
class USkyAtmosphereComponent;
class USkyLightComponent;
struct FKataDayNightLightCurves;

/**
 * 현재 시각에 맞춰 레벨의 하늘 액터를 바꾸는 낮밤 연출 액터. 레벨에 하나 배치한다.
 *
 * 게임·PIE에서는 UKataWorldTimeSubsystem::GetTimeOfDay를 매 프레임 읽고, 마지막으로 적용한 시각과 다를 때만 DayNightData의 커브를 평가해 적용한다.
 * 시간 상태를 바꾸지 않고 델리게이트도 구독하지 않는다. Fixed 모드에서도 시각 지정으로 값이 바뀌면 다시 적용한다.
 * 시각이 그대로이거나 게임이 일시정지 중이면 다시 적용하지 않는다.
 * 편집 중인 레벨에서는 bPreviewInEditor가 켜져 있을 때 PreviewHour를 적용한다. 미리 본 값은 대상 액터의 속성에 남아 맵을 저장하면 함께 저장되고,
 * 게임·PIE를 시작하면 현재 시각의 값으로 덮어쓴다.
 *
 * 대상 참조가 비어 있으면 이 액터와 같은 레벨에서 찾는다. 태양은 Atmosphere Sun Light가 켜져 있고 Index가 0인 Directional Light,
 * 달도 Atmosphere Sun Light가 켜져 있고 Index가 1인 Directional Light를 찾는다. 나머지는 클래스별로 처음 찾은 액터다. 찾은 결과는 참조 칸에 써넣지 않는다.
 * 게임 월드에서는 Mobility가 Movable인 Directional Light만 회전할 수 있다. Movable이 아니면 회전을 건너뛰고 세기와 색만 적용한다.
 * 데이터에 회전 커브가 있는 조명이 Movable이 아니면 BeginPlay에서 경고한다.
 * Sky Light는 실시간 캡처를 전제로 하며 다시 캡처를 요청하지 않는다. 실시간 캡처가 꺼져 있으면 BeginPlay에서 경고한다.
 * 싱글플레이 전용이며 복제하지 않는다.
 */
UCLASS(meta = (DisplayName = "Kata Day Night Director"))
class KATAWORLD_API AKataDayNightDirector : public AInfo
{
    GENERATED_BODY()

public:
    AKataDayNightDirector();

    /** 적용할 시각별 커브. 비어 있으면 대상 액터를 바꾸지 않는다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Day Night")
    TObjectPtr<UKataDayNightData> DayNightData;

    /** 이 레벨에서 태양과 달의 방위에 더하는 각도(도). 레벨마다 해가 뜨는 방향을 바꿀 때 쓴다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|Day Night", meta = (ForceUnits = "Degrees"))
    float YawOffset = 0.0f;

    /** 태양 조명. 비어 있으면 Atmosphere Sun Light가 켜져 있고 Index가 0인 Directional Light를 같은 레벨에서 찾는다. */
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Kata|Day Night|Targets")
    TObjectPtr<ADirectionalLight> SunLight;

    /** 달 조명. 비어 있으면 Atmosphere Sun Light가 켜져 있고 Index가 1인 Directional Light를 같은 레벨에서 찾는다. 없으면 달 커브를 쓰지 않는다. */
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Kata|Day Night|Targets")
    TObjectPtr<ADirectionalLight> MoonLight;

    /** Sky Light. 비어 있으면 같은 레벨에서 처음 찾은 Sky Light를 쓴다. */
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Kata|Day Night|Targets")
    TObjectPtr<ASkyLight> SkyLight;

    /** Sky Atmosphere. 비어 있으면 같은 레벨에서 처음 찾은 Sky Atmosphere를 쓴다. */
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Kata|Day Night|Targets")
    TObjectPtr<ASkyAtmosphere> SkyAtmosphere;

    /** Exponential Height Fog. 비어 있으면 같은 레벨에서 처음 찾은 Exponential Height Fog를 쓴다. */
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Kata|Day Night|Targets")
    TObjectPtr<AExponentialHeightFog> HeightFog;

#if WITH_EDITORONLY_DATA
    /** 켜면 편집 중인 레벨에 PreviewHour를 적용한다. 끄면 대상 액터를 더 바꾸지 않으며 마지막으로 적용한 값이 그대로 남는다. */
    UPROPERTY(EditAnywhere, Category = "Kata|Day Night|Preview")
    bool bPreviewInEditor = true;

    /** 편집 중인 레벨에서 미리 볼 시각(시). 게임·PIE의 시각에는 영향이 없다. */
    UPROPERTY(EditAnywhere, Category = "Kata|Day Night|Preview", meta = (EditCondition = "bPreviewInEditor", ClampMin = "0.0", ClampMax = "24.0", UIMax = "24.0", ForceUnits = "Hours"))
    float PreviewHour = 9.0f;
#endif

    virtual void Tick(float DeltaSeconds) override;
    virtual void OnConstruction(const FTransform& Transform) override;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    /** 참조 칸과 같은 레벨의 액터에서 적용 대상 컴포넌트를 정한다. 태양과 같은 액터를 달로 지정했으면 달을 비운다. */
    void ResolveTargets();

    /** 게임 월드에서 의도대로 적용할 수 없는 설정을 경고한다. ResolveTargets 뒤에 부른다. */
    void WarnInvalidSetup() const;

    /** Hour에서 평가한 커브 값을 대상에 적용하고 LastAppliedHour를 갱신한다. */
    void ApplyHour(float Hour);

    /** Directional Light 하나에 회전, 세기, 색을 적용한다. 게임 월드에서 Movable이 아니면 회전은 건너뛴다. */
    void ApplyLight(UDirectionalLightComponent* Light, const FKataDayNightLightCurves& Curves, float Hour) const;

    /** 대상 컴포넌트는 레벨의 다른 액터가 소유하므로 약한 참조로 둔다. 사라진 대상은 건너뛴다. */
    TWeakObjectPtr<UDirectionalLightComponent> SunComponent;
    TWeakObjectPtr<UDirectionalLightComponent> MoonComponent;
    TWeakObjectPtr<USkyLightComponent> SkyLightComponent;
    TWeakObjectPtr<USkyAtmosphereComponent> SkyAtmosphereComponent;
    TWeakObjectPtr<UExponentialHeightFogComponent> HeightFogComponent;

    /** 마지막으로 적용한 시각. 아직 적용하지 않았으면 음수다. */
    float LastAppliedHour = -1.0f;
};
