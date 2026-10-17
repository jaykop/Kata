#include "DayNight/KataDayNightDirector.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "DayNight/KataDayNightData.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/Level.h"
#include "Engine/SkyLight.h"
#include "Engine/World.h"
#include "KataWorldLog.h"
#include "Time/KataWorldTimeSubsystem.h"

namespace
{
    /**
     * 안개·대기의 세터는 값이 조금만 달라도 렌더 상태를 다시 만든다. 시간이 흐르는 동안 매 프레임 다시 만들지 않도록
     * 실수 값은 새 값의 절댓값 × 0.005와 UE_KINDA_SMALL_NUMBER 중 큰 값 이하의 차이를 건너뛴다.
     * 색은 모든 채널의 차이가 0.005 이하면 건너뛴다. 조명·Sky Light 세터는 이 임계값 없이 부른다.
     */
    constexpr float RenderStateRelativeTolerance = 0.005f;
    constexpr float RenderStateColorTolerance = 0.005f;

    /** 커브에 키나 기본값이 있으면 Hour에서 평가한다. 비어 있으면 false를 돌려주고 대상 속성을 바꾸지 않는다. */
    bool EvaluateCurve(const FRuntimeFloatCurve& Curve, float Hour, float& OutValue)
    {
        const FRichCurve* RichCurve = Curve.GetRichCurveConst();
        if (RichCurve == nullptr || !RichCurve->HasAnyData())
        {
            return false;
        }
        OutValue = RichCurve->Eval(Hour);
        return true;
    }

    /** 외부 커브가 있거나 R·G·B 중 하나라도 데이터가 있으면 Hour에서 평가한다. Alpha만 있는 커브는 빈 커브로 본다. */
    bool EvaluateColor(const FRuntimeCurveLinearColor& Curve, float Hour, FLinearColor& OutColor)
    {
        const bool bHasData = Curve.ExternalCurve != nullptr
            || Curve.ColorCurves[0].HasAnyData()
            || Curve.ColorCurves[1].HasAnyData()
            || Curve.ColorCurves[2].HasAnyData();
        if (!bHasData)
        {
            return false;
        }
        OutColor = Curve.GetLinearColorValue(Hour);
        return true;
    }

    bool HasVisibleChange(float Current, float New)
    {
        return !FMath::IsNearlyEqual(Current, New, FMath::Max(FMath::Abs(New) * RenderStateRelativeTolerance, UE_KINDA_SMALL_NUMBER));
    }

    bool HasVisibleChange(const FLinearColor& Current, const FLinearColor& New)
    {
        return !Current.Equals(New, RenderStateColorTolerance);
    }

    bool HasRotationCurve(const FKataDayNightLightCurves& Curves)
    {
        const FRichCurve* Elevation = Curves.Elevation.GetRichCurveConst();
        const FRichCurve* Azimuth = Curves.Azimuth.GetRichCurveConst();
        return (Elevation != nullptr && Elevation->HasAnyData()) || (Azimuth != nullptr && Azimuth->HasAnyData());
    }

    UDirectionalLightComponent* GetDirectionalLightComponent(const ADirectionalLight* Light)
    {
        // ADirectionalLight::GetComponent는 에디터 전용이라 게임 빌드에서 쓸 수 없다.
        return Light != nullptr ? Cast<UDirectionalLightComponent>(Light->GetLightComponent()) : nullptr;
    }
}

AKataDayNightDirector::AKataDayNightDirector()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;
}

void AKataDayNightDirector::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    const UKataWorldTimeSubsystem* TimeSubsystem = UKataWorldTimeSubsystem::Get(this);
    if (TimeSubsystem == nullptr)
    {
        return;
    }

    // 시각이 멈춰 있으면 Subsystem이 같은 float 값을 돌려주므로 정확히 같을 때만 건너뛴다.
    const float Hour = TimeSubsystem->GetTimeOfDay();
    if (Hour != LastAppliedHour)
    {
        ApplyHour(Hour);
    }
}

void AKataDayNightDirector::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

#if WITH_EDITOR
    // 엔진은 Details 값을 바꿀 때마다(슬라이더를 드래그하는 중에도) 생성 스크립트를 다시 실행하므로 여기서 미리보기를 적용한다.
    // 게임 월드에서 생성될 때와 커맨드릿(쿠킹 등)에서는 미리보기를 적용하지 않는다.
    const UWorld* World = GetWorld();
    if (bPreviewInEditor && World != nullptr && World->WorldType == EWorldType::Editor && !IsRunningCommandlet())
    {
        ResolveTargets();
        ApplyHour(PreviewHour);
    }
#endif
}

void AKataDayNightDirector::BeginPlay()
{
    Super::BeginPlay();

    ResolveTargets();
    WarnInvalidSetup();

    LastAppliedHour = -1.0f;
    if (const UKataWorldTimeSubsystem* TimeSubsystem = UKataWorldTimeSubsystem::Get(this))
    {
        ApplyHour(TimeSubsystem->GetTimeOfDay());
    }
}

void AKataDayNightDirector::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    SunComponent.Reset();
    MoonComponent.Reset();
    SkyLightComponent.Reset();
    SkyAtmosphereComponent.Reset();
    HeightFogComponent.Reset();

    Super::EndPlay(EndPlayReason);
}

void AKataDayNightDirector::ResolveTargets()
{
    ADirectionalLight* FoundSun = SunLight;
    ADirectionalLight* FoundMoon = MoonLight;
    ASkyLight* FoundSkyLight = SkyLight;
    ASkyAtmosphere* FoundSkyAtmosphere = SkyAtmosphere;
    AExponentialHeightFog* FoundHeightFog = HeightFog;

    const ULevel* Level = GetLevel();
    const bool bNeedSearch = FoundSun == nullptr || FoundMoon == nullptr || FoundSkyLight == nullptr || FoundSkyAtmosphere == nullptr || FoundHeightFog == nullptr;
    if (bNeedSearch && Level != nullptr)
    {
        for (AActor* Actor : Level->Actors)
        {
            if (!IsValid(Actor))
            {
                continue;
            }

            if (ADirectionalLight* Light = Cast<ADirectionalLight>(Actor))
            {
                // 대기 광원이 아닌 Directional Light(연출용 보조광 등)는 태양·달로 고르지 않는다. 참조 칸에 지정한 태양은 달 후보에서 뺀다.
                const UDirectionalLightComponent* LightComponent = GetDirectionalLightComponent(Light);
                if (LightComponent == nullptr || !LightComponent->bAtmosphereSunLight || Light == FoundSun)
                {
                    continue;
                }
                if (FoundSun == nullptr && LightComponent->AtmosphereSunLightIndex == 0)
                {
                    FoundSun = Light;
                }
                else if (FoundMoon == nullptr && LightComponent->AtmosphereSunLightIndex == 1)
                {
                    FoundMoon = Light;
                }
            }
            else if (FoundSkyLight == nullptr && Actor->IsA<ASkyLight>())
            {
                FoundSkyLight = CastChecked<ASkyLight>(Actor);
            }
            else if (FoundSkyAtmosphere == nullptr && Actor->IsA<ASkyAtmosphere>())
            {
                FoundSkyAtmosphere = CastChecked<ASkyAtmosphere>(Actor);
            }
            else if (FoundHeightFog == nullptr && Actor->IsA<AExponentialHeightFog>())
            {
                FoundHeightFog = CastChecked<AExponentialHeightFog>(Actor);
            }
        }
    }

    // 같은 조명을 태양과 달로 지정하면 두 커브가 서로 덮어쓰므로 달을 쓰지 않는다. 경고는 WarnInvalidSetup이 남긴다.
    if (FoundMoon == FoundSun)
    {
        FoundMoon = nullptr;
    }

    SunComponent = GetDirectionalLightComponent(FoundSun);
    MoonComponent = GetDirectionalLightComponent(FoundMoon);
    SkyLightComponent = FoundSkyLight != nullptr ? FoundSkyLight->GetLightComponent() : nullptr;
    SkyAtmosphereComponent = FoundSkyAtmosphere != nullptr ? FoundSkyAtmosphere->GetComponent() : nullptr;
    HeightFogComponent = FoundHeightFog != nullptr ? FoundHeightFog->GetComponent() : nullptr;
}

void AKataDayNightDirector::WarnInvalidSetup() const
{
    if (DayNightData == nullptr)
    {
        UE_LOG(LogKataWorld, Warning, TEXT("Day night director %s has no data and changes nothing"), *GetNameSafe(this));
        return;
    }

    if (SunLight != nullptr && SunLight == MoonLight)
    {
        UE_LOG(LogKataWorld, Warning, TEXT("Day night director %s: %s is set as both sun and moon; moon curves are ignored"),
            *GetNameSafe(this), *GetNameSafe(SunLight));
    }

    if (!SunComponent.IsValid())
    {
        UE_LOG(LogKataWorld, Warning, TEXT("Day night director %s found no sun directional light (atmosphere sun light index 0)"), *GetNameSafe(this));
    }

    auto WarnIfNotMovable = [this](const UDirectionalLightComponent* Light, const FKataDayNightLightCurves& Curves)
    {
        if (Light != nullptr && Light->Mobility != EComponentMobility::Movable && HasRotationCurve(Curves))
        {
            UE_LOG(LogKataWorld, Warning, TEXT("Day night director %s: %s is not Movable and will not rotate at runtime"),
                *GetNameSafe(this), *GetNameSafe(Light->GetOwner()));
        }
    };
    WarnIfNotMovable(SunComponent.Get(), DayNightData->Sun);
    WarnIfNotMovable(MoonComponent.Get(), DayNightData->Moon);

    if (const USkyLightComponent* SkyLightPtr = SkyLightComponent.Get(); SkyLightPtr != nullptr && !SkyLightPtr->IsRealTimeCaptureEnabled())
    {
        UE_LOG(LogKataWorld, Warning, TEXT("Day night director %s: %s does not use real-time capture; sky lighting will not follow the time of day"),
            *GetNameSafe(this), *GetNameSafe(SkyLightPtr->GetOwner()));
    }
}

void AKataDayNightDirector::ApplyHour(float Hour)
{
    LastAppliedHour = Hour;

    const UKataDayNightData* Data = DayNightData;
    if (Data == nullptr)
    {
        return;
    }

    ApplyLight(SunComponent.Get(), Data->Sun, Hour);
    ApplyLight(MoonComponent.Get(), Data->Moon, Hour);

    float Value = 0.0f;
    FLinearColor Color;

    if (USkyLightComponent* SkyLightPtr = SkyLightComponent.Get())
    {
        if (EvaluateCurve(Data->SkyLightIntensity, Hour, Value))
        {
            SkyLightPtr->SetIntensity(FMath::Max(Value, 0.0f));
        }
        if (EvaluateColor(Data->SkyLightColor, Hour, Color))
        {
            SkyLightPtr->SetLightColor(Color);
        }
    }

    if (USkyAtmosphereComponent* Atmosphere = SkyAtmosphereComponent.Get())
    {
        if (EvaluateCurve(Data->RayleighScatteringScale, Hour, Value) && HasVisibleChange(Atmosphere->RayleighScatteringScale, FMath::Max(Value, 0.0f)))
        {
            Atmosphere->SetRayleighScatteringScale(FMath::Max(Value, 0.0f));
        }
        if (EvaluateCurve(Data->MieScatteringScale, Hour, Value) && HasVisibleChange(Atmosphere->MieScatteringScale, FMath::Max(Value, 0.0f)))
        {
            Atmosphere->SetMieScatteringScale(FMath::Max(Value, 0.0f));
        }
        if (EvaluateColor(Data->SkyLuminanceFactor, Hour, Color) && HasVisibleChange(Atmosphere->SkyLuminanceFactor, Color))
        {
            Atmosphere->SetSkyLuminanceFactor(Color);
        }
    }

    if (UExponentialHeightFogComponent* Fog = HeightFogComponent.Get())
    {
        if (EvaluateCurve(Data->FogDensity, Hour, Value) && HasVisibleChange(Fog->FogDensity, FMath::Max(Value, 0.0f)))
        {
            Fog->SetFogDensity(FMath::Max(Value, 0.0f));
        }
        // 안개의 "Color" 세터는 FogInscatteringLuminance·DirectionalInscatteringLuminance에 값을 쓴다.
        if (EvaluateColor(Data->FogInscatteringColor, Hour, Color) && HasVisibleChange(Fog->FogInscatteringLuminance, Color))
        {
            Fog->SetFogInscatteringColor(Color);
        }
        if (EvaluateColor(Data->DirectionalInscatteringColor, Hour, Color) && HasVisibleChange(Fog->DirectionalInscatteringLuminance, Color))
        {
            Fog->SetDirectionalInscatteringColor(Color);
        }
    }
}

void AKataDayNightDirector::ApplyLight(UDirectionalLightComponent* Light, const FKataDayNightLightCurves& Curves, float Hour) const
{
    if (Light == nullptr)
    {
        return;
    }

    float Elevation = 0.0f;
    float Azimuth = 0.0f;
    const bool bHasElevation = EvaluateCurve(Curves.Elevation, Hour, Elevation);
    const bool bHasAzimuth = EvaluateCurve(Curves.Azimuth, Hour, Azimuth);

    // 게임 월드에서 Movable이 아닌 컴포넌트를 옮기면 엔진이 매번 경고하고 무시하므로 미리 건너뛴다. 편집 월드에서는 미리보기를 위해 회전한다.
    const UWorld* World = GetWorld();
    const bool bCanRotate = Light->Mobility == EComponentMobility::Movable || World == nullptr || !World->IsGameWorld();
    if ((bHasElevation || bHasAzimuth) && bCanRotate)
    {
        // 조명은 광원의 반대쪽을 향한다. 지평선 위 고도는 아래로 향하는 Pitch, 방위는 180도 돌린 Yaw가 된다.
        FRotator Rotation = Light->GetComponentRotation();
        if (bHasElevation)
        {
            Rotation.Pitch = -Elevation;
        }
        if (bHasAzimuth)
        {
            Rotation.Yaw = Azimuth + 180.0f + YawOffset;
        }
        Light->SetWorldRotation(Rotation);
    }

    float Intensity = 0.0f;
    if (EvaluateCurve(Curves.Intensity, Hour, Intensity))
    {
        Light->SetIntensity(FMath::Max(Intensity, 0.0f));
    }

    FLinearColor Color;
    if (EvaluateColor(Curves.Color, Hour, Color))
    {
        Light->SetLightColor(Color);
    }
}
