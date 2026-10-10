#include "Tasks/KataTask_TargetTilt.h"

#include "Animation/KataAnimInstance.h"
#include "Animation/KataTiltComponent.h"
#include "Animation/KataTiltDebug.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "Runtime/KataActionInstance.h"
#include "Targeting/KataTargetingComponent.h"

namespace
{
    /** 액터 몸통의 세로 범위. 루트가 캡슐이 아니면 바닥과 꼭대기가 모두 액터 위치 높이이고 절반 높이는 0이다. */
    struct FKataVerticalExtent
    {
        double Bottom = 0.0;
        double Top = 0.0;
        double HalfHeight = 0.0;
    };

    FKataVerticalExtent GetVerticalExtent(const AActor& Actor)
    {
        const UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(Actor.GetRootComponent());
        const double HalfHeight = Capsule != nullptr ? Capsule->GetScaledCapsuleHalfHeight() : 0.0;
        const double CenterZ = Actor.GetActorLocation().Z;
        return { CenterZ - HalfHeight, CenterZ + HalfHeight, HalfHeight };
    }

    /** 메시의 Anim Instance가 UKataAnimInstance면 돌려준다. 캐릭터는 캐릭터 메시를, 아니면 처음 찾은 스켈레탈 메시를 본다. */
    const UKataAnimInstance* FindKataAnimInstance(const AActor& Actor)
    {
        const ACharacter* Character = Cast<ACharacter>(&Actor);
        const USkeletalMeshComponent* Mesh = Character != nullptr ? Character->GetMesh() : Actor.FindComponentByClass<USkeletalMeshComponent>();
        return Mesh != nullptr ? Cast<UKataAnimInstance>(Mesh->GetAnimInstance()) : nullptr;
    }

#if ENABLE_DRAW_DEBUG
    /**
     * 조준 원점(흰색)에서 가정 조준점(회색)과 실제 조준점(노랑)까지 선을 긋는다. 두 선 사이의 각이 Tilt 목표다.
     * 대상이 정면 밖이라 기울이지 않을 때는 실제 조준선을 빨강으로 그린다. 매 Tick 그리고 한 프레임만 남는다.
     */
    void DrawTiltAimDebug(const UWorld* World, const FVector& Origin, const FVector& Expected, const FVector& Actual,
        float RawPitch, float TargetPitch, bool bTargetInFront)
    {
        if (World == nullptr)
        {
            return;
        }

        const FColor ExpectedColor(160, 160, 160);
        const FColor ActualColor = bTargetInFront ? FColor::Yellow : FColor::Red;
        DrawDebugSphere(World, Origin, 6.0f, 8, FColor::White);
        DrawDebugLine(World, Origin, Expected, ExpectedColor);
        DrawDebugSphere(World, Expected, 6.0f, 8, ExpectedColor);
        DrawDebugLine(World, Origin, Actual, ActualColor);
        DrawDebugSphere(World, Actual, 6.0f, 8, ActualColor);

        const FString Text = bTargetInFront
            ? FString::Printf(TEXT("Target %.1f (raw %.1f)"), TargetPitch, RawPitch)
            : FString(TEXT("Target behind, no tilt"));
        // 문자열은 Duration -1이 만료 없음이라 쌓이므로 한 번 그리고 지워지는 0을 쓴다.
        DrawDebugString(World, Actual + FVector(0.0, 0.0, 20.0), Text, nullptr, ActualColor, 0.0f, true);
    }
#endif
}

UKataTask_TargetTilt::UKataTask_TargetTilt()
{
    // 공격의 준비부터 휘두름까지를 덮는 길이를 기본값으로 둔다.
    Duration = 0.5f;
    Phase = EKataTaskPhase::Animation;
}

TSubclassOf<UKataTaskInstance> UKataTask_TargetTilt::GetTaskInstanceClass_Implementation() const
{
    return UKataTaskInstance_TargetTilt::StaticClass();
}

FName UKataTask_TargetTilt::GetConfigurationError() const
{
    const FName SuperError = Super::GetConfigurationError();
    if (!SuperError.IsNone())
    {
        return SuperError;
    }
    if (bSingleFrame || !(Duration > 0.0f))
    {
        // 한 프레임으로는 블렌드도 추적도 할 수 없다.
        return TEXT("ZeroLengthTargetTilt");
    }
    return NAME_None;
}

FString UKataTask_TargetTilt::DescribeConfigurationError(FName ErrorCode) const
{
    if (ErrorCode == TEXT("ZeroLengthTargetTilt"))
    {
        return TEXT("Target Tilt needs a duration covering the attack; 'Single Frame' and zero 'Duration' are not allowed");
    }
    return Super::DescribeConfigurationError(ErrorCode);
}

void UKataTaskInstance_TargetTilt::OnTaskStarted_Implementation()
{
    const UKataTask_TargetTilt* Definition = Cast<UKataTask_TargetTilt>(GetTaskDefinition());
    const FKataContext Context = GetKataContext();
    AActor* Avatar = Context.GetAvatarActor();
    AActor* Target = Context.GetTargetActor();
    UKataTiltComponent* Component = Avatar != nullptr ? Avatar->FindComponentByClass<UKataTiltComponent>() : nullptr;
    if (Definition == nullptr || Component == nullptr || !IsValid(Target) || Target == Avatar)
    {
        FinishTask();
        return;
    }

    TiltActor = Avatar;
    TargetActor = Target;
    Targeting = Avatar->FindComponentByClass<UKataTargetingComponent>();
    TiltComponent = Component;
    TrackElapsed = 0.0f;

    // 높이 설정은 애니메이션 세트에 묶인 값이라 캐릭터별 자식 ABP의 Class Defaults에 둔다.
    const UKataAnimInstance* AnimInstance = FindKataAnimInstance(*Avatar);
    AimOriginHeight = AnimInstance != nullptr ? AnimInstance->AimOriginHeight : 0.0f;
    ExpectedTargetHeight = AnimInstance != nullptr ? AnimInstance->ExpectedTargetHeight : 0.0f;
    if (Definition->bOverrideExpectedTargetHeight)
    {
        ExpectedTargetHeight = Definition->ExpectedTargetHeight;
    }

    float TargetPitch = 0.0f;
    ComputeTargetPitch(TargetPitch);

    FKataTiltBlendSettings Settings;
    Settings.BlendInTime = Definition->BlendInTime;
    Settings.BlendOutTime = Definition->BlendOutTime;
    Settings.MaxPitchSpeed = Definition->MaxPitchSpeed;
    TiltHandle = Component->BeginTilt(Settings, TargetPitch);
}

void UKataTaskInstance_TargetTilt::OnTaskTick_Implementation(float DeltaTime)
{
    UKataTiltComponent* Component = TiltComponent.Get();
    const UKataTask_TargetTilt* Definition = Cast<UKataTask_TargetTilt>(GetTaskDefinition());
    if (Component == nullptr || Definition == nullptr)
    {
        FinishTask();
        return;
    }

    // 추적이 끝나면 마지막 목표를 그대로 두어 구간 끝까지 그 각도를 유지한다.
    TrackElapsed += DeltaTime;
    const bool bTracking = Definition->bTrackUntilEnd || TrackElapsed <= Definition->TrackDuration;
#if ENABLE_DRAW_DEBUG
    const bool bDrawDebug = KataTiltDebug::IsDrawEnabled();
#else
    const bool bDrawDebug = false;
#endif
    if (!bTracking && !bDrawDebug)
    {
        return;
    }

    // 대상이 사라져 각도를 구하지 못하면 직전 목표를 유지한다.
    // 추적이 끝난 뒤의 계산은 고정한 각도와 지금 대상 위치를 비교해 보여 주는 디버그 표시용이며 목표를 바꾸지 않는다.
    float TargetPitch = 0.0f;
    if (ComputeTargetPitch(TargetPitch, bDrawDebug) && bTracking)
    {
        Component->SetTiltTarget(TiltHandle, TargetPitch);
    }
}

void UKataTaskInstance_TargetTilt::OnTaskEnded_Implementation(EKataTaskEndReason Reason)
{
    // 어떤 사유로 끝나도 해제한다. 블렌드 아웃은 컴포넌트가 이어서 처리한다.
    if (UKataTiltComponent* Component = TiltComponent.Get())
    {
        Component->EndTilt(TiltHandle);
    }
    TiltComponent.Reset();
    TiltActor.Reset();
    TargetActor.Reset();
    Targeting.Reset();
    TiltHandle = INDEX_NONE;

    Super::OnTaskEnded_Implementation(Reason);
}

bool UKataTaskInstance_TargetTilt::ComputeTargetPitch(float& OutPitch, bool bDrawDebug) const
{
    const UKataTask_TargetTilt* Definition = Cast<UKataTask_TargetTilt>(GetTaskDefinition());
    const AActor* Avatar = TiltActor.Get();
    AActor* Target = TargetActor.Get();
    if (Definition == nullptr || Avatar == nullptr || !IsValid(Target))
    {
        return false;
    }

    FVector AimLocation = FVector::ZeroVector;
    bool bIsTargetPoint = false;
    const UKataTargetingComponent* TargetingComponent = Targeting.Get();
    if (TargetingComponent == nullptr || !TargetingComponent->ResolveAimLocation(Target, AimLocation, bIsTargetPoint))
    {
        AimLocation = Target->GetActorLocation();
        bIsTargetPoint = false;
    }

    const FVector SelfLocation = Avatar->GetActorLocation();
    FVector ToTarget = AimLocation - SelfLocation;
    ToTarget.Z = 0.0;
    const double HorizontalDistance = ToTarget.Size();

    // Pitch는 앞뒤를 구분하지 않는다. 몸 정면에서 수평 90도 넘게 벗어난 대상 쪽으로 기울이면 반대 방향으로 숙여 보이므로 기울이지 않는다.
    FVector Forward = Avatar->GetActorForwardVector();
    Forward.Z = 0.0;
    const bool bTargetInFront = HorizontalDistance <= UE_KINDA_SMALL_NUMBER || FVector::DotProduct(ToTarget, Forward) > 0.0;

    // 원점과 가정 조준점은 내 발 높이를 기준으로 잡는다. 0인 설정은 자기 캡슐 절반 높이(같은 체격의 중심)로 본다.
    const FKataVerticalExtent Self = GetVerticalExtent(*Avatar);
    const double OriginHeight = AimOriginHeight > 0.0f ? AimOriginHeight : Self.HalfHeight;
    const double ExpectedHeight = ExpectedTargetHeight > 0.0f ? ExpectedTargetHeight : Self.HalfHeight;
    const double OriginZ = Self.Bottom + OriginHeight;
    const double ExpectedZ = Self.Bottom + ExpectedHeight;

    // 부위 지점은 그 높이를 그대로 겨눈다. 아니면 대상 몸통의 세로 범위에서 기준 키에 가장 가까운 점을 겨눠,
    // 거대한 대상은 다리 쪽을, 작은 대상은 꼭대기를 겨누게 한다. 그래서 체격 차이만으로 크게 기울지 않는다.
    double ActualZ = AimLocation.Z;
    if (!bIsTargetPoint)
    {
        const FKataVerticalExtent TargetExtent = GetVerticalExtent(*Target);
        ActualZ = FMath::Clamp(TargetExtent.Bottom + ExpectedHeight, TargetExtent.Bottom, TargetExtent.Top);
    }

    const double Distance = FMath::Max(HorizontalDistance, static_cast<double>(Definition->MinHorizontalDistance));
    const double PitchRadians = FMath::Atan2(ActualZ - OriginZ, Distance) - FMath::Atan2(ExpectedZ - OriginZ, Distance);
    const float RawPitch = bTargetInFront ? static_cast<float>(FMath::RadiansToDegrees(PitchRadians)) : 0.0f;
    OutPitch = FMath::Clamp(RawPitch, -Definition->MaxPitchDown, Definition->MaxPitchUp);

#if ENABLE_DRAW_DEBUG
    if (bDrawDebug)
    {
        const FVector Origin(SelfLocation.X, SelfLocation.Y, OriginZ);
        const FVector Expected(AimLocation.X, AimLocation.Y, ExpectedZ);
        const FVector Actual(AimLocation.X, AimLocation.Y, ActualZ);
        DrawTiltAimDebug(Avatar->GetWorld(), Origin, Expected, Actual, RawPitch, OutPitch, bTargetInFront);
    }
#endif
    return true;
}
