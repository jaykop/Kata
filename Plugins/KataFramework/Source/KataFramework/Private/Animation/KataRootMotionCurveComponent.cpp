#include "Animation/KataRootMotionCurveComponent.h"

#include "Animation/AnimationAsset.h"
#include "Animation/AnimCompositeBase.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Animation/KataFL_RootMotionCurve.h"
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

    using FTrackRanges = TArray<TPair<float, float>, TInlineAllocator<4>>;

    /**
     * 이번 갱신에서 몽타주가 지나간 트랙 구간을 섹션 단위로 나눈다.
     * Advance는 시작 위치와 총 이동량만 남기므로, 시작 위치 + 이동량이 현재 위치와 다르면 섹션이 바뀐 것으로 보고
     * 섹션 링크를 따라 경로를 다시 만든다. 다시 만든 경로가 현재 위치와 맞지 않거나 역재생이면 false를 반환한다.
     */
    bool BuildTrackRanges(const FAnimMontageInstance& Instance, FTrackRanges& OutRanges)
    {
        const UAnimMontage* Montage = Instance.Montage;
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

        float Position = Previous;
        float Remaining = Moved;
        int32 SectionIndex = Montage->GetSectionIndexFromPosition(Position);
        for (int32 Step = 0; Step < MaxSectionSteps && SectionIndex != INDEX_NONE; ++Step)
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

            SectionIndex = Instance.GetNextSectionID(SectionIndex);
            if (SectionIndex == INDEX_NONE)
            {
                break;
            }
            Montage->GetSectionStartAndEndTime(SectionIndex, SectionStart, SectionEnd);
            Position = SectionStart;
        }

        return Remaining <= TrackPositionTolerance && FMath::IsNearlyEqual(Position, Current, TrackPositionTolerance);
    }

    /**
     * 몽타주 트랙 구간 하나의 루트 모션 변화량을 만든다.
     * 몽타주 커브가 있으면 몽타주 트랙 시각으로 바로 읽고, 없으면 엔진 추출과 같은 단계로 나눠 시퀀스마다
     * 커브 또는 원래 루트 모션을 쓴다. 커브를 하나라도 썼으면 bOutUsedCurve를 true로 바꾼다.
     */
    FTransform ExtractTrackRange(const UAnimMontage& Montage, float Start, float End, bool bUseMontageCurves, bool& bOutUsedCurve)
    {
        if (bUseMontageCurves)
        {
            FKataRootMotionCurveValue StartValue;
            FKataRootMotionCurveValue EndValue;
            if (KataFL::EvaluateRootMotionCurves(Montage, Start, StartValue) && KataFL::EvaluateRootMotionCurves(Montage, End, EndValue))
            {
                bOutUsedCurve = true;
                return KataFL::MakeRootMotionCurveDelta(StartValue, EndValue);
            }
        }

        // UAnimCompositeBase::ExtractRootMotionFromTrack과 같은 순서로 단계를 누적해야 이동이 회전 기준으로 맞게 쌓인다.
        TArray<FRootMotionExtractionStep> Steps;
        Montage.SlotAnimTracks[0].AnimTrack.GetRootMotionExtractionStepsForTrackRange(Steps, Start, End);

        FRootMotionMovementParams Accumulated;
        for (const FRootMotionExtractionStep& Step : Steps)
        {
            const UAnimSequence* Sequence = Step.AnimSequence;
            if (Sequence == nullptr || !Sequence->bEnableRootMotion)
            {
                continue;
            }

            FKataRootMotionCurveValue StartValue;
            FKataRootMotionCurveValue EndValue;
            if (KataFL::EvaluateRootMotionCurves(*Sequence, Step.StartPosition, StartValue)
                && KataFL::EvaluateRootMotionCurves(*Sequence, Step.EndPosition, EndValue))
            {
                Accumulated.Accumulate(KataFL::MakeRootMotionCurveDelta(StartValue, EndValue));
                bOutUsedCurve = true;
            }
            else
            {
                Accumulated.Accumulate(Sequence->ExtractRootMotionFromRange(Step.StartPosition, Step.EndPosition, FAnimExtractContext()));
            }
        }
        return Accumulated.GetRootMotionTransform();
    }
}

UKataRootMotionCurveComponent::UKataRootMotionCurveComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
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
            TEXT("Kata root motion curve component on '%s' found ProcessRootMotionPreConvertToWorld already bound; root motion curves are disabled for this character"),
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

    Super::OnUnregister();
}

FTransform UKataRootMotionCurveComponent::ProcessRootMotion(const FTransform& InRootMotion, UCharacterMovementComponent* Movement, float DeltaSeconds)
{
    if (!bUseRootMotionCurves || Movement == nullptr)
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
        return InRootMotion;
    }

    const bool bUseMontageCurves = KataFL::HasRootMotionCurves(*Montage);
    bool bUsedCurve = false;
    FRootMotionMovementParams Motion;
    for (const TPair<float, float>& Range : Ranges)
    {
        Motion.Accumulate(ExtractTrackRange(*Montage, Range.Key, Range.Value, bUseMontageCurves, bUsedCurve));
    }

    if (!bUsedCurve)
    {
        return InRootMotion;
    }

    // 엔진 입력값에는 이미 이동 배율이 곱해져 있으므로 대체값에도 같은 배율을 적용한다(UCharacterMovementComponent::TickCharacterPose).
    FTransform Result = Motion.GetRootMotionTransform();
    Result.ScaleTranslation(Character->GetAnimRootMotionTranslationScale());
    return Result;
}
