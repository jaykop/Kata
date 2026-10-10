#include "Animation/KataRootMotionCurveComponent.h"

#include "Animation/AnimationAsset.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/KataFL_RootMotionCurve.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "KataFrameworkLog.h"

namespace
{
    /** 트랙 위치 비교 허용치(초). 엔진이 섹션 끝에서 쓰는 보정값보다 충분히 크다. */
    constexpr float TrackPositionTolerance = 1.e-3f;

    /** FAnimMontageInstance::Advance의 한 갱신 반복 한도와 같다. */
    constexpr int32 MaxSectionSteps = 10;

    /** 남은 이동이 대상 쪽으로 이보다 짧으면(cm) 배율을 구하지 않는다. 0에 가까운 값으로 나누면 배율이 튄다. */
    constexpr float MinForwardDistance = 1.0f;

    using FTrackRanges = TArray<TPair<float, float>, TInlineAllocator<4>>;

    /**
     * StartPosition부터 트랙 길이 Length만큼 섹션 링크를 따라 나아가며 지나는 구간을 OutRanges에 더한다.
     * 다음 섹션이 없으면 몽타주가 그 섹션 끝에서 멈추므로 거기서 그친다(FAnimMontageInstance::Advance).
     *
     * @return 나아가지 못하고 남은 길이. 끝까지 나아갔으면 0이다.
     */
    float WalkTrackRanges(const FAnimMontageInstance& Instance, float StartPosition, float Length, FTrackRanges& OutRanges, float& OutEndPosition)
    {
        const UAnimMontage* Montage = Instance.Montage;
        float Position = StartPosition;
        float Remaining = Length;
        int32 SectionIndex = Montage->GetSectionIndexFromPosition(Position);
        for (int32 Step = 0; Step < MaxSectionSteps && SectionIndex != INDEX_NONE && Remaining > 0.0f; ++Step)
        {
            float SectionStart = 0.0f;
            float SectionEnd = 0.0f;
            Montage->GetSectionStartAndEndTime(SectionIndex, SectionStart, SectionEnd);

            const float Available = SectionEnd - Position;
            if (Remaining <= Available + TrackPositionTolerance)
            {
                OutRanges.Emplace(Position, Position + Remaining);
                Position += Remaining;
                Remaining = 0.0f;
                break;
            }

            OutRanges.Emplace(Position, SectionEnd);
            Remaining -= Available;
            Position = SectionEnd;

            SectionIndex = Instance.GetNextSectionID(SectionIndex);
            if (SectionIndex == INDEX_NONE)
            {
                break;
            }
            Montage->GetSectionStartAndEndTime(SectionIndex, SectionStart, SectionEnd);
            Position = SectionStart;
        }

        OutEndPosition = Position;
        return Remaining;
    }

    /**
     * 이번 갱신에서 몽타주가 지나간 트랙 구간을 섹션 단위로 나눈다.
     * Advance는 시작 위치와 총 이동량만 남기므로, 시작 위치 + 이동량이 현재 위치와 다르면 섹션이 바뀐 것으로 보고
     * 섹션 링크를 따라 경로를 다시 만든다. 다시 만든 경로가 현재 위치와 맞지 않거나 역재생이면 false를 반환한다.
     */
    bool BuildTrackRanges(const FAnimMontageInstance& Instance, FTrackRanges& OutRanges)
    {
        const float Previous = Instance.GetPreviousPosition();
        const float Moved = Instance.GetDeltaMoved();
        const float Current = Instance.GetPosition();

        if (FMath::IsNearlyZero(Moved))
        {
            return true;
        }
        if (Moved < 0.0f)
        {
            // 역방향 섹션 링크는 공개 접근 경로가 없어 1단계에서는 엔진 값을 쓴다.
            return false;
        }
        if (FMath::IsNearlyEqual(Previous + Moved, Current, TrackPositionTolerance))
        {
            // 섹션이 바뀌지 않았거나 다음 섹션이 바로 이어 붙어 있다. 세그먼트 경계는 추출 단계가 나눈다.
            OutRanges.Emplace(Previous, Current);
            return true;
        }

        float EndPosition = Previous;
        const float Unwalked = WalkTrackRanges(Instance, Previous, Moved, OutRanges, EndPosition);
        return Unwalked <= TrackPositionTolerance && FMath::IsNearlyEqual(EndPosition, Current, TrackPositionTolerance);
    }

    /** Ranges의 앞쪽 트랙 길이 Length까지를 OutFirst에, 나머지를 OutSecond에 나눠 담는다. */
    void SplitTrackRanges(TConstArrayView<TPair<float, float>> Ranges, float Length, FTrackRanges& OutFirst, FTrackRanges& OutSecond)
    {
        float Remaining = Length;
        for (const TPair<float, float>& Range : Ranges)
        {
            const float RangeLength = Range.Value - Range.Key;
            if (Remaining >= RangeLength)
            {
                OutFirst.Add(Range);
                Remaining -= RangeLength;
            }
            else if (Remaining > 0.0f)
            {
                const float Split = Range.Key + Remaining;
                OutFirst.Emplace(Range.Key, Split);
                OutSecond.Emplace(Split, Range.Value);
                Remaining = 0.0f;
            }
            else
            {
                OutSecond.Add(Range);
            }
        }
    }

    /**
     * 트랙 구간들의 루트 모션을 차례로 이어 붙인다. bUseCurves가 false면 커브가 있어도 원래 루트 모션을 쓴다.
     * 커브를 하나라도 썼으면 bOutUsedCurve를 true로 바꾼다.
     */
    FTransform ExtractTrackRanges(const UAnimMontage& Montage, TConstArrayView<TPair<float, float>> Ranges, bool bUseCurves, bool& bOutUsedCurve)
    {
        const bool bUseMontageCurves = bUseCurves && KataFL::HasRootMotionCurves(Montage);
        FRootMotionMovementParams Motion;
        for (const TPair<float, float>& Range : Ranges)
        {
            Motion.Accumulate(KataFL::ExtractMontageRootMotion(Montage, Range.Key, Range.Value, bUseMontageCurves, bOutUsedCurve, bUseCurves));
        }
        return Motion.GetRootMotionTransform();
    }

    /**
     * 메시 공간 이동의 월드 수평 성분에 Scale을 곱한다. 수직 성분과 회전은 그대로 둔다.
     * 델리게이트 입력은 메시 컴포넌트 공간이므로(USkeletalMeshComponent::ConvertLocalRootMotionToWorld) 월드로 바꿔 곱하고 되돌린다.
     */
    FTransform ScaleHorizontalTranslation(FTransform Motion, const FTransform& MeshTransform, float Scale)
    {
        FVector WorldTranslation = MeshTransform.TransformVector(Motion.GetTranslation());
        WorldTranslation.X *= Scale;
        WorldTranslation.Y *= Scale;
        Motion.SetTranslation(MeshTransform.InverseTransformVector(WorldTranslation));
        return Motion;
    }

    /** 메시 공간 이동의 월드 수평 길이(cm). 최소·최대 전진 거리에 누적한다. */
    float GetHorizontalLength(const FTransform& Motion, const FTransform& MeshTransform)
    {
        return MeshTransform.TransformVector(Motion.GetTranslation()).Size2D();
    }
}

UKataRootMotionCurveComponent::UKataRootMotionCurveComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

int32 UKataRootMotionCurveComponent::BeginDistanceCorrection(FKataRootMotionDistanceRequest Request)
{
    FActiveDistanceCorrection Correction;
    Correction.Request = MoveTemp(Request);
    Correction.Handle = NextDistanceCorrectionHandle++;
    DistanceCorrection = MoveTemp(Correction);

    if (!BoundMovement.IsValid())
    {
        UE_LOG(LogKataFramework, Warning,
            TEXT("Kata root motion curve component on '%s' is not bound to CharacterMovement; distance correction has no effect"),
            *GetNameSafe(GetOwner()));
    }
    return DistanceCorrection->Handle;
}

void UKataRootMotionCurveComponent::EndDistanceCorrection(int32 Handle)
{
    if (DistanceCorrection.IsSet() && DistanceCorrection->Handle == Handle)
    {
        DistanceCorrection.Reset();
    }
}

void UKataRootMotionCurveComponent::OnRegister()
{
    Super::OnRegister();

    const ACharacter* Character = Cast<ACharacter>(GetOwner());
    UCharacterMovementComponent* Movement = Character != nullptr ? Character->GetCharacterMovement() : nullptr;
    if (Movement == nullptr)
    {
        return;
    }

    if (Movement->ProcessRootMotionPreConvertToWorld.IsBound() && !Movement->ProcessRootMotionPreConvertToWorld.IsBoundToObject(this))
    {
        // 단일 바인딩 델리게이트라 덮어쓰면 다른 시스템의 루트 모션 처리가 조용히 사라진다.
        UE_LOG(LogKataFramework, Warning,
            TEXT("Kata root motion curve component on '%s' found ProcessRootMotionPreConvertToWorld already bound; root motion curves and distance correction are disabled for this character"),
            *GetNameSafe(GetOwner()));
        return;
    }

    Movement->ProcessRootMotionPreConvertToWorld.BindUObject(this, &UKataRootMotionCurveComponent::ProcessRootMotion);
    BoundMovement = Movement;
}

void UKataRootMotionCurveComponent::OnUnregister()
{
    if (UCharacterMovementComponent* Movement = BoundMovement.Get())
    {
        if (Movement->ProcessRootMotionPreConvertToWorld.IsBoundToObject(this))
        {
            Movement->ProcessRootMotionPreConvertToWorld.Unbind();
        }
    }
    BoundMovement.Reset();
    DistanceCorrection.Reset();

    Super::OnUnregister();
}

FTransform UKataRootMotionCurveComponent::ProcessRootMotion(const FTransform& InRootMotion, UCharacterMovementComponent* Movement, float DeltaSeconds)
{
    if (Movement == nullptr || (!bUseRootMotionCurves && !DistanceCorrection.IsSet()))
    {
        return InRootMotion;
    }

    const ACharacter* Character = Movement->GetCharacterOwner();
    const USkeletalMeshComponent* Mesh = Character != nullptr ? Character->GetMesh() : nullptr;
    const UAnimInstance* AnimInstance = Mesh != nullptr ? Mesh->GetAnimInstance() : nullptr;
    if (AnimInstance == nullptr || AnimInstance->RootMotionMode != ERootMotionMode::RootMotionFromMontagesOnly)
    {
        // Root Motion From Everything는 몽타주 밖의 루트 모션도 섞으므로 몽타주 구간만으로 대체할 수 없다.
        return InRootMotion;
    }

    // Root Motion From Montages Only에서는 루트 모션 몽타주 하나만 가중치 없이 루트 모션을 낸다(UAnimInstance::Montage_Advance).
    const FAnimMontageInstance* Instance = AnimInstance->GetRootMotionMontageInstance();
    const UAnimMontage* Montage = Instance != nullptr ? Instance->Montage.Get() : nullptr;
    if (Montage == nullptr || !Instance->IsPlaying() || Instance->IsRootMotionDisabled() || !Montage->HasRootMotion()
        || Montage->SlotAnimTracks.IsEmpty())
    {
        return InRootMotion;
    }

    FTrackRanges Ranges;
    if (!BuildTrackRanges(*Instance, Ranges))
    {
        // 지나간 경로를 알 수 없으면 남은 구간도 셀 수 없으므로 거리 보정도 멈춘다.
        DistanceCorrection.Reset();
        return InRootMotion;
    }

    // 1단계: 커브 대체.
    FTransform Result = InRootMotion;
    if (bUseRootMotionCurves)
    {
        bool bUsedCurve = false;
        const FTransform CurveMotion = ExtractTrackRanges(*Montage, Ranges, true, bUsedCurve);
        if (bUsedCurve)
        {
            // 엔진 입력값에는 이미 이동 배율이 곱해져 있으므로 대체값에도 같은 배율을 적용한다(UCharacterMovementComponent::TickCharacterPose).
            Result = CurveMotion;
            Result.ScaleTranslation(Character->GetAnimRootMotionTranslationScale());
        }
    }

    // 2단계: 거리 보정.
    if (DistanceCorrection.IsSet())
    {
        Result = ApplyDistanceCorrection(Result, *Character, *Mesh, *Instance, Ranges);
    }

    return Result;
}

FTransform UKataRootMotionCurveComponent::ApplyDistanceCorrection(const FTransform& FrameMotion, const ACharacter& Character,
    const USkeletalMeshComponent& Mesh, const FAnimMontageInstance& Instance, TConstArrayView<TPair<float, float>> FrameRanges)
{
    FActiveDistanceCorrection& Correction = DistanceCorrection.GetValue();
    const UAnimMontage& Montage = *Instance.Montage;

    if (Correction.MontageInstanceId == INDEX_NONE)
    {
        // 같은 시각에 시작하면 Movement 단계 태스크가 Animation 단계의 몽타주 재생보다 먼저 시작하므로,
        // 요청 시점이 아니라 블렌드 아웃 중이 아닌 루트 모션 몽타주를 처음 만난 갱신에서 창을 고정한다.
        if (Instance.IsStopped())
        {
            return FrameMotion;
        }
        Correction.MontageInstanceId = Instance.GetInstanceID();
        Correction.RemainingWindowLength = Correction.Request.Duration * FMath::Abs(Instance.GetPlayRate() * Montage.RateScale);
    }
    else if (Correction.MontageInstanceId != Instance.GetInstanceID())
    {
        // 창을 고정한 재생이 끝나고 다른 몽타주가 루트 모션을 낸다. 남은 구간이 이어지지 않으므로 보정을 끝낸다.
        DistanceCorrection.Reset();
        return FrameMotion;
    }

    const float Moved = Instance.GetDeltaMoved();
    if (Moved <= 0.0f)
    {
        return FrameMotion;
    }

    const float WindowLength = Correction.RemainingWindowLength;
    Correction.RemainingWindowLength -= Moved;
    if (WindowLength <= TrackPositionTolerance)
    {
        DistanceCorrection.Reset();
        return FrameMotion;
    }

    // 남은 구간 전체의 이동으로 배율을 매 갱신 다시 구해야 창 끝에서 오차가 모이지 않는다.
    // 남은 이동량은 1단계와 같은 원천(커브 또는 원래 루트 모션)에서 구한다.
    FTrackRanges RemainingRanges;
    float RemainingEnd = 0.0f;
    WalkTrackRanges(Instance, Instance.GetPreviousPosition(), WindowLength, RemainingRanges, RemainingEnd);

    bool bUsedCurve = false;
    const float TranslationScale = Character.GetAnimRootMotionTranslationScale();
    const FTransform& MeshTransform = Mesh.GetComponentTransform();
    const FVector RemainingLocal = ExtractTrackRanges(Montage, RemainingRanges, bUseRootMotionCurves, bUsedCurve).GetTranslation() * TranslationScale;
    FVector RemainingWorld = MeshTransform.TransformVector(RemainingLocal);
    RemainingWorld.Z = 0.0f;

    float Scale = 1.0f;
    if (!ComputeDistanceScale(Correction, Character, RemainingWorld, Scale))
    {
        return FrameMotion;
    }

    if (Moved <= WindowLength + TrackPositionTolerance)
    {
        const FTransform Corrected = ScaleHorizontalTranslation(FrameMotion, MeshTransform, Scale);
        Correction.TraveledDistance += GetHorizontalLength(Corrected, MeshTransform);
        return Corrected;
    }

    // 창 끝을 지나는 갱신은 창 안 부분만 늘이거나 줄이고, 창 밖 부분은 원래 이동을 이어 붙인다.
    FTrackRanges InsideRanges;
    FTrackRanges OutsideRanges;
    SplitTrackRanges(FrameRanges, WindowLength, InsideRanges, OutsideRanges);

    FTransform Inside = ExtractTrackRanges(Montage, InsideRanges, bUseRootMotionCurves, bUsedCurve);
    FTransform Outside = ExtractTrackRanges(Montage, OutsideRanges, bUseRootMotionCurves, bUsedCurve);
    Inside.ScaleTranslation(TranslationScale);
    Outside.ScaleTranslation(TranslationScale);

    // 창 밖 부분은 이번 갱신을 끝으로 요청이 끝나므로 전진 거리에 더하지 않는다.
    const FTransform CorrectedInside = ScaleHorizontalTranslation(Inside, MeshTransform, Scale);
    Correction.TraveledDistance += GetHorizontalLength(CorrectedInside, MeshTransform);

    FRootMotionMovementParams Motion;
    Motion.Accumulate(CorrectedInside);
    Motion.Accumulate(Outside);
    return Motion.GetRootMotionTransform();
}

bool UKataRootMotionCurveComponent::ComputeDistanceScale(FActiveDistanceCorrection& Correction, const ACharacter& Character,
    const FVector& RemainingWorld, float& OutScale) const
{
    const FKataRootMotionDistanceRequest& Request = Correction.Request;

    FVector TargetLocation = Correction.FixedTargetLocation;
    float TargetRadius = Correction.FixedTargetRadius;
    if (Request.bTrackTarget || !Correction.bHasFixedTarget)
    {
        if (!Request.ResolveTarget || !Request.ResolveTarget(TargetLocation, TargetRadius))
        {
            return false;
        }
        if (!Request.bTrackTarget)
        {
            Correction.bHasFixedTarget = true;
            Correction.FixedTargetLocation = TargetLocation;
            Correction.FixedTargetRadius = TargetRadius;
        }
    }

    FVector ToTarget = TargetLocation - Character.GetActorLocation();
    ToTarget.Z = 0.0f;
    const float Distance = ToTarget.Size();
    if (Distance <= UE_KINDA_SMALL_NUMBER)
    {
        return false;
    }

    // 대상에서 멀어지거나 옆으로 비껴가는 이동은 늘여도 대상에 다가가지 않으므로 보정하지 않는다.
    const float PathLength = RemainingWorld.Size();
    const float Forward = FVector::DotProduct(RemainingWorld, ToTarget / Distance);
    if (Forward < MinForwardDistance || PathLength < MinForwardDistance)
    {
        return false;
    }

    const UCapsuleComponent* Capsule = Character.GetCapsuleComponent();
    const float SelfRadius = Capsule != nullptr ? Capsule->GetScaledCapsuleRadius() : 0.0f;
    const float Desired = FMath::Max(0.0f, Distance - SelfRadius - TargetRadius - Request.StopDistance);

    // 방향은 회전 태스크가 맡으므로 경로 방향은 그대로 두고, 그 경로 위에서 대상에 가장 가까워지는 지점까지만 간다.
    // 경로가 대상과 이루는 각을 θ라 하면 이동 거리는 Desired × cosθ다. 대상 쪽 성분만으로 배율을 정하면
    // 몸이 대상에서 크게 틀어져 있을 때 배율이 커져 엉뚱한 방향으로 멀리 돌진한다.
    const float CosAngle = Forward / PathLength;
    const float WantedTravel = Desired * CosAngle;

    // 최소·최대 전진 거리는 구간 전체의 실제 수평 이동 총량이므로 이미 이동한 거리를 빼 남은 구간의 허용 범위를 만든다.
    const float MaxRemaining = FMath::Max(0.0f, Request.MaxDistance - Correction.TraveledDistance);
    const float MinRemaining = FMath::Clamp(Request.MinDistance - Correction.TraveledDistance, 0.0f, MaxRemaining);
    OutScale = FMath::Clamp(WantedTravel, MinRemaining, MaxRemaining) / PathLength;
    return true;
}
