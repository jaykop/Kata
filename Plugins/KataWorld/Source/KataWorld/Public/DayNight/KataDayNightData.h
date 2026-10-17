#pragma once

#include "CoreMinimal.h"
#include "Curves/CurveFloat.h"
#include "Curves/CurveLinearColor.h"
#include "Engine/DataAsset.h"
#include "KataDayNightData.generated.h"

/**
 * Directional Light 하나(태양 또는 달)에 쓰는 시각별 커브 묶음.
 *
 * 모든 커브의 가로축은 시각(0 이상 24 미만의 시)이다. 자정에서 값이 끊기지 않도록 0시와 24시의 키를 같은 상태로 맞춘다.
 * 비어 있는 커브는 그 속성을 바꾸지 않는다. Elevation과 Azimuth 중 하나만 비어 있으면 그 성분은 조명의 현재 회전을 유지한다.
 */
USTRUCT()
struct KATAWORLD_API FKataDayNightLightCurves
{
    GENERATED_BODY()

    /** 광원이 하늘에 보이는 고도(도). 양수면 지평선 위, 음수면 아래다. 조명의 Pitch는 이 값의 부호를 바꾼 값이다. */
    UPROPERTY(EditAnywhere, Category = "Light")
    FRuntimeFloatCurve Elevation;

    /**
     * 광원이 하늘에 보이는 수평 방향(도). 조명은 그 반대 방향을 향하므로 Yaw는 이 값에 180도와 연출 액터의 Yaw Offset을 더한 값이다.
     * 하루에 한 바퀴 도는 움직임은 0시 0도에서 24시 360도처럼 이어서 적는다. 0시와 24시가 360도 차이면 같은 방향이다.
     */
    UPROPERTY(EditAnywhere, Category = "Light")
    FRuntimeFloatCurve Azimuth;

    /** 조명 세기(lux). 0보다 작은 값은 0으로 적용한다. */
    UPROPERTY(EditAnywhere, Category = "Light")
    FRuntimeFloatCurve Intensity;

    /** 조명 색(선형). 조명의 Use Temperature가 켜져 있으면 색온도에서 얻은 색과 곱해진다. */
    UPROPERTY(EditAnywhere, Category = "Light")
    FRuntimeCurveLinearColor Color;
};

/**
 * 시각에 따라 레벨의 하늘 액터를 바꾸는 커브 모음. AKataDayNightDirector가 현재 시각에서 평가해
 * 태양·달 Directional Light, Sky Light, Sky Atmosphere, Exponential Height Fog에 적용한다.
 *
 * 모든 커브의 가로축은 시각(0 이상 24 미만의 시)이다. 비어 있는 커브는 해당 속성을 바꾸지 않으므로 필요한 항목만 채운다.
 * 새 에셋에는 태양과 달의 Elevation, Azimuth, Intensity, Sky Light Intensity, Sky Luminance Factor에 기본 키가 들어 있어 따로 편집하지 않아도 하루가 돈다.
 * 노을의 색은 Sky Atmosphere가 태양 고도에 따라 만들므로 조명 색 커브는 기본으로 비워 둔다. 안개와 나머지 대기 커브도 비어 있다.
 * 여러 레벨이 공유하는 에셋이므로 실행 중 상태를 저장하지 않는다.
 */
UCLASS(BlueprintType, Const, meta = (DisplayName = "Kata Day Night Data"))
class KATAWORLD_API UKataDayNightData : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UKataDayNightData();

    /** 태양 Directional Light에 적용하는 커브. */
    UPROPERTY(EditAnywhere, Category = "Sun")
    FKataDayNightLightCurves Sun;

    /** 달 Directional Light에 적용하는 커브. 레벨에 달 조명이 없으면 쓰지 않는다. */
    UPROPERTY(EditAnywhere, Category = "Moon")
    FKataDayNightLightCurves Moon;

    /** Sky Light 세기 배율. 0보다 작은 값은 0으로 적용한다. */
    UPROPERTY(EditAnywhere, Category = "Sky Light")
    FRuntimeFloatCurve SkyLightIntensity;

    /** Sky Light 색(선형). 캡처한 하늘빛에 곱해진다. */
    UPROPERTY(EditAnywhere, Category = "Sky Light")
    FRuntimeCurveLinearColor SkyLightColor;

    /** Exponential Height Fog의 전체 밀도. 0보다 작은 값은 0으로 적용한다. */
    UPROPERTY(EditAnywhere, Category = "Fog")
    FRuntimeFloatCurve FogDensity;

    /** 안개가 하늘빛을 산란해 띠는 색(선형). 안개 컴포넌트의 Fog Inscattering Color에 적용한다. */
    UPROPERTY(EditAnywhere, Category = "Fog")
    FRuntimeCurveLinearColor FogInscatteringColor;

    /** 태양 쪽을 볼 때 안개에 더해지는 색(선형). 안개 컴포넌트의 Directional Inscattering Color에 적용한다. */
    UPROPERTY(EditAnywhere, Category = "Fog")
    FRuntimeCurveLinearColor DirectionalInscatteringColor;

    /** Sky Atmosphere의 Rayleigh 산란 배율. 낮 하늘의 파란 정도를 바꾼다. 0보다 작은 값은 0으로 적용한다. */
    UPROPERTY(EditAnywhere, Category = "Atmosphere")
    FRuntimeFloatCurve RayleighScatteringScale;

    /** Sky Atmosphere의 Mie 산란 배율. 태양 주변의 뿌연 빛 번짐을 바꾼다. 0보다 작은 값은 0으로 적용한다. */
    UPROPERTY(EditAnywhere, Category = "Atmosphere")
    FRuntimeFloatCurve MieScatteringScale;

    /** Sky Atmosphere의 Sky Luminance Factor(선형). 하늘 밝기와 색조에 곱해진다. */
    UPROPERTY(EditAnywhere, Category = "Atmosphere")
    FRuntimeCurveLinearColor SkyLuminanceFactor;
};
