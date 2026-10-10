#include "Animation/KataRootMotionCurveModifier.h"

#include "Animation/AnimData/CurveIdentifier.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "Animation/AnimData/IAnimationDataModel.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimationAsset.h"
#include "Animation/KataFL_RootMotionCurve.h"
#include "Curves/RichCurve.h"

#define LOCTEXT_NAMESPACE "KataRootMotionCurveModifier"

DEFINE_LOG_CATEGORY_STATIC(LogKataRootMotionCurve, Log, All);

namespace
{
    /** 첫 프레임부터 Time까지의 원본 루트 모션. 실행 시 몽타주가 쓰는 추출 함수와 같은 경로로 읽는다. */
    FTransform ExtractRootFromStart(const UAnimSequence& Animation, double Time)
    {
        return Animation.ExtractRootMotionFromRange(0.0, Time, FAnimExtractContext());
    }

    /** 이전 값에 이어지도록 Yaw를 펼친다. Rotator의 Yaw는 ±180도에서 끊기기 때문이다. */
    double UnwrapYaw(double RawYaw, double PreviousYaw)
    {
        return PreviousYaw + FMath::UnwindDegrees(RawYaw - PreviousYaw);
    }
}

void UKataRootMotionCurveModifier::OnApply_Implementation(UAnimSequence* Animation)
{
    if (Animation == nullptr)
    {
        return;
    }

    const double PlayLength = Animation->GetPlayLength();
    if (!(PlayLength > 0.0))
    {
        UE_LOG(LogKataRootMotionCurve, Warning, TEXT("Kata root motion curve: '%s' has no play length; nothing was extracted"), *Animation->GetName());
        return;
    }

    if (!Animation->HasRootMotion())
    {
        // 실행 시 대체는 엔진 루트 모션 경로 안에서 일어나므로 Enable Root Motion이 꺼져 있으면 커브가 쓰이지 않는다.
        UE_LOG(LogKataRootMotionCurve, Warning,
            TEXT("Kata root motion curve: '%s' has Enable Root Motion off; the curves will not drive movement until it is enabled"),
            *Animation->GetName());
    }
    if (bReapplyPostOwnerChange)
    {
        UE_LOG(LogKataRootMotionCurve, Warning,
            TEXT("Kata root motion curve: 'Reapply Post Owner Change' is on for '%s'; any later edit to the asset re-extracts and overwrites edited curves"),
            *Animation->GetName());
    }

    // 간격을 같게 나눠 마지막 키가 재생 길이와 정확히 맞게 한다.
    // 시퀀스 프레임 레이트를 쓰면 키 시각이 원본 프레임과 겹쳐 선형 보간이 엔진의 루트 본 보간과 같아진다.
    const double KeysPerSecond = bUseSequenceFrameRate ? Animation->GetSamplingFrameRate().AsDecimal() : static_cast<double>(SampleRate);
    const int32 NumIntervals = FMath::Max(1, FMath::CeilToInt32(PlayLength * FMath::Max(KeysPerSecond, 1.0) - UE_KINDA_SMALL_NUMBER));

    TArray<FKataRootMotionCurveValue> Samples;
    TArray<double> SampleTimes;
    Samples.Reserve(NumIntervals + 1);
    SampleTimes.Reserve(NumIntervals + 1);

    double MaxOffAxisRotation = 0.0;
    for (int32 Index = 0; Index <= NumIntervals; ++Index)
    {
        const double Time = PlayLength * Index / NumIntervals;
        const FTransform Root = ExtractRootFromStart(*Animation, Time);
        const FRotator Rotation = Root.Rotator();

        FKataRootMotionCurveValue Value;
        Value.Translation = Root.GetTranslation();
        Value.Yaw = Samples.IsEmpty() ? Rotation.Yaw : UnwrapYaw(Rotation.Yaw, Samples.Last().Yaw);
        MaxOffAxisRotation = FMath::Max3(MaxOffAxisRotation, FMath::Abs(Rotation.Pitch), FMath::Abs(Rotation.Roll));

        Samples.Add(Value);
        SampleTimes.Add(Time);
    }

    const TArray<FName> CurveNames = KataFL::GetRootMotionCurveNames();
    TArray<FRichCurveKey> CurveKeys[4];
    for (int32 Index = 0; Index < Samples.Num(); ++Index)
    {
        const float Time = static_cast<float>(SampleTimes[Index]);
        const FKataRootMotionCurveValue& Value = Samples[Index];
        CurveKeys[0].Add(FRichCurveKey(Time, static_cast<float>(Value.Translation.X)));
        CurveKeys[1].Add(FRichCurveKey(Time, static_cast<float>(Value.Translation.Y)));
        CurveKeys[2].Add(FRichCurveKey(Time, static_cast<float>(Value.Translation.Z)));
        CurveKeys[3].Add(FRichCurveKey(Time, static_cast<float>(Value.Yaw)));
    }

    {
        IAnimationDataController& Controller = Animation->GetController();
        IAnimationDataController::FScopedBracket Bracket(Controller, LOCTEXT("ApplyRootMotionCurves", "Apply Kata Root Motion Curves"));
        for (int32 CurveIndex = 0; CurveIndex < CurveNames.Num(); ++CurveIndex)
        {
            const FAnimationCurveIdentifier CurveId(CurveNames[CurveIndex], ERawCurveTrackTypes::RCT_Float);
            if (Animation->GetDataModel()->FindFloatCurve(CurveId) == nullptr)
            {
                Controller.AddCurve(CurveId);
            }
            // 기존 커브는 키 전체를 바꿔 덮어쓴다. 재추출은 사용자가 Apply를 실행한 경우에만 일어난다.
            Controller.SetCurveKeys(CurveId, CurveKeys[CurveIndex]);
        }
    }

    // 원본과 일치하는지 샘플 사이 중간 시각에서 비교한다. 키 시각에서는 정의상 같으므로 보간 오차만 남는다.
    double MaxPositionError = 0.0;
    double MaxYawError = 0.0;
    for (int32 Index = 0; Index < NumIntervals; ++Index)
    {
        const double MidTime = (SampleTimes[Index] + SampleTimes[Index + 1]) * 0.5;
        const FTransform Original = ExtractRootFromStart(*Animation, MidTime);
        const FVector CurveTranslation = FMath::Lerp(Samples[Index].Translation, Samples[Index + 1].Translation, 0.5);
        const double CurveYaw = FMath::Lerp(Samples[Index].Yaw, Samples[Index + 1].Yaw, 0.5);

        MaxPositionError = FMath::Max(MaxPositionError, FVector::Dist(Original.GetTranslation(), CurveTranslation));
        MaxYawError = FMath::Max(MaxYawError, FMath::Abs(FMath::UnwindDegrees(Original.Rotator().Yaw - CurveYaw)));
    }

    const FKataRootMotionCurveValue& End = Samples.Last();
    UE_LOG(LogKataRootMotionCurve, Display,
        TEXT("Kata root motion curve: extracted '%s' with %d keys (end translation %s, end yaw %.2f, max interval error %.3f cm / %.3f deg)"),
        *Animation->GetName(), Samples.Num(), *End.Translation.ToCompactString(), End.Yaw, MaxPositionError, MaxYawError);

    if (MaxPositionError > PositionTolerance || MaxYawError > RotationTolerance)
    {
        UE_LOG(LogKataRootMotionCurve, Warning,
            TEXT("Kata root motion curve: '%s' differs from the original root motion between keys (%.3f cm / %.3f deg); use the sequence frame rate or a higher sample rate"),
            *Animation->GetName(), MaxPositionError, MaxYawError);
    }
    if (MaxOffAxisRotation > RotationTolerance)
    {
        UE_LOG(LogKataRootMotionCurve, Warning,
            TEXT("Kata root motion curve: '%s' has root pitch/roll up to %.2f deg; the curves keep yaw only"),
            *Animation->GetName(), MaxOffAxisRotation);
    }
}

void UKataRootMotionCurveModifier::OnRevert_Implementation(UAnimSequence* Animation)
{
    if (Animation == nullptr)
    {
        return;
    }

    IAnimationDataController& Controller = Animation->GetController();
    IAnimationDataController::FScopedBracket Bracket(Controller, LOCTEXT("RevertRootMotionCurves", "Revert Kata Root Motion Curves"));
    for (const FName CurveName : KataFL::GetRootMotionCurveNames())
    {
        const FAnimationCurveIdentifier CurveId(CurveName, ERawCurveTrackTypes::RCT_Float);
        if (Animation->GetDataModel()->FindFloatCurve(CurveId) != nullptr)
        {
            Controller.RemoveCurve(CurveId);
        }
    }
}

#undef LOCTEXT_NAMESPACE
