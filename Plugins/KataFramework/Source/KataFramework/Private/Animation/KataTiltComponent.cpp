#include "Animation/KataTiltComponent.h"

#include "Animation/KataTiltDebug.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "HAL/IConsoleManager.h"
#include "Runtime/KataActionComponent.h"

#if ENABLE_DRAW_DEBUG
namespace
{
    TAutoConsoleVariable<int32> CVarKataTiltDebug(
        TEXT("Kata.Tilt.Debug"),
        0,
        TEXT("Draw Kata target tilt debug. 0: off, 1: aim origin, expected aim point (gray), actual aim point (yellow, red when the target is behind), ")
        TEXT("target pitch and the applied pitch/alpha above the character."),
        ECVF_Cheat);

    TAutoConsoleVariable<float> CVarKataTiltForcePitch(
        TEXT("Kata.Tilt.ForcePitch"),
        0.0f,
        TEXT("Force the Kata Tilt pitch in degrees (positive is up) with full alpha on every Kata Anim Instance, ignoring targets and tasks. ")
        TEXT("Use it to check the Kata Tilt node and the bone chain. 0: off."),
        ECVF_Cheat);
}

bool KataTiltDebug::IsDrawEnabled()
{
    return CVarKataTiltDebug.GetValueOnGameThread() > 0;
}

bool KataTiltDebug::GetForcedPitch(float& OutPitch)
{
    const float Value = CVarKataTiltForcePitch.GetValueOnGameThread();
    if (FMath::IsNearlyZero(Value))
    {
        return false;
    }
    OutPitch = Value;
    return true;
}
#endif

namespace
{
    /** Current를 Target 쪽으로 BlendTime 동안 0~1 전체를 지나는 속도로 옮긴다. BlendTime이 0 이하면 바로 Target이다. */
    float StepAlpha(float Current, float Target, float BlendTime, float DeltaTime)
    {
        if (!(BlendTime > 0.0f))
        {
            return Target;
        }
        return FMath::FInterpConstantTo(Current, Target, DeltaTime, 1.0f / BlendTime);
    }
}

UKataTiltComponent::UKataTiltComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

int32 UKataTiltComponent::BeginTilt(const FKataTiltBlendSettings& Settings, float TargetPitch)
{
    if (CurrentAlpha <= 0.0f)
    {
        CurrentPitch = TargetPitch;
    }

    FActiveTilt& Tilt = ActiveTilts.AddDefaulted_GetRef();
    Tilt.Handle = NextHandle++;
    Tilt.Settings = Settings;
    Tilt.TargetPitch = TargetPitch;

    SetComponentTickEnabled(true);
    return Tilt.Handle;
}

void UKataTiltComponent::SetTiltTarget(int32 Handle, float TargetPitch)
{
    for (FActiveTilt& Tilt : ActiveTilts)
    {
        if (Tilt.Handle == Handle)
        {
            Tilt.TargetPitch = TargetPitch;
            return;
        }
    }
}

void UKataTiltComponent::EndTilt(int32 Handle)
{
    const int32 Index = ActiveTilts.IndexOfByPredicate([Handle](const FActiveTilt& Tilt) { return Tilt.Handle == Handle; });
    if (Index == INDEX_NONE)
    {
        return;
    }

    // 마지막 요청이 빠지면 그 요청의 블렌드 아웃으로 내린다. 중간 요청이 빠지면 따르던 요청이 그대로라 바뀌는 것이 없다.
    if (ActiveTilts.Num() == 1)
    {
        FadeOutTime = ActiveTilts[Index].Settings.BlendOutTime;
    }
    ActiveTilts.RemoveAt(Index);
}

void UKataTiltComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

#if ENABLE_DRAW_DEBUG
    // 지난 Tick의 출력 값이다. 이번 Tick의 갱신은 다음 프레임에 보인다.
    // DrawDebugString의 Duration -1은 HUD가 만료 없음으로 처리해 매 프레임 쌓이므로, 한 번 그리고 지워지는 0을 쓴다.
    const AActor* Owner = GetOwner();
    if (Owner != nullptr && KataTiltDebug::IsDrawEnabled())
    {
        const FVector TextLocation = Owner->GetActorLocation() + FVector(0.0, 0.0, Owner->GetSimpleCollisionHalfHeight() + 30.0);
        DrawDebugString(GetWorld(), TextLocation, FString::Printf(TEXT("Tilt %.1f  Alpha %.2f"), CurrentPitch, CurrentAlpha),
            nullptr, FColor::Cyan, 0.0f, true);
    }
#endif

    if (!ActiveTilts.IsEmpty())
    {
        const FActiveTilt& Tilt = ActiveTilts.Last();
        CurrentAlpha = StepAlpha(CurrentAlpha, 1.0f, Tilt.Settings.BlendInTime, DeltaTime);
        CurrentPitch = Tilt.Settings.MaxPitchSpeed > 0.0f
            ? FMath::FInterpConstantTo(CurrentPitch, Tilt.TargetPitch, DeltaTime, Tilt.Settings.MaxPitchSpeed)
            : Tilt.TargetPitch;
        return;
    }

    // 블렌드 아웃 동안에는 마지막 Pitch를 유지한다. Pitch까지 0으로 내리면 Alpha와 겹쳐 더 빨리 풀린 것처럼 보인다.
    CurrentAlpha = StepAlpha(CurrentAlpha, 0.0f, FadeOutTime, DeltaTime);
    if (CurrentAlpha <= 0.0f)
    {
        CurrentAlpha = 0.0f;
        CurrentPitch = 0.0f;
        SetComponentTickEnabled(false);
    }
}

void UKataTiltComponent::BeginPlay()
{
    Super::BeginPlay();

    SetTickPrerequisites(true);
}

void UKataTiltComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    SetTickPrerequisites(false);

    Super::EndPlay(EndPlayReason);
}

void UKataTiltComponent::OnUnregister()
{
    ResetTilt();

    Super::OnUnregister();
}

void UKataTiltComponent::ResetTilt()
{
    ActiveTilts.Reset();
    CurrentPitch = 0.0f;
    CurrentAlpha = 0.0f;
    FadeOutTime = 0.0f;
    SetComponentTickEnabled(false);
}

void UKataTiltComponent::SetTickPrerequisites(bool bAdd)
{
    AActor* Owner = GetOwner();
    if (Owner == nullptr)
    {
        return;
    }

    // 꺼진 Tick은 선행 관계에서 무시되므로, 요청이 없어 Tick을 끈 동안에도 메시 Tick은 막히지 않는다.
    if (UKataActionComponent* ActionComponent = Owner->FindComponentByClass<UKataActionComponent>())
    {
        if (bAdd)
        {
            PrimaryComponentTick.AddPrerequisite(ActionComponent, ActionComponent->PrimaryComponentTick);
        }
        else
        {
            PrimaryComponentTick.RemovePrerequisite(ActionComponent, ActionComponent->PrimaryComponentTick);
        }
    }

    const ACharacter* Character = Cast<ACharacter>(Owner);
    if (USkeletalMeshComponent* Mesh = Character != nullptr ? Character->GetMesh() : nullptr)
    {
        if (bAdd)
        {
            Mesh->PrimaryComponentTick.AddPrerequisite(this, PrimaryComponentTick);
        }
        else
        {
            Mesh->PrimaryComponentTick.RemovePrerequisite(this, PrimaryComponentTick);
        }
    }
}
