#include "Animation/KataFL_RootMotionCurve.h"

#include "Animation/AnimationAsset.h"
#include "Animation/AnimSequenceBase.h"

FName KataFL::GetRootMotionCurveNameX()
{
    static const FName Name(TEXT("Kata.RootMotion.X"));
    return Name;
}

FName KataFL::GetRootMotionCurveNameY()
{
    static const FName Name(TEXT("Kata.RootMotion.Y"));
    return Name;
}

FName KataFL::GetRootMotionCurveNameZ()
{
    static const FName Name(TEXT("Kata.RootMotion.Z"));
    return Name;
}

FName KataFL::GetRootMotionCurveNameYaw()
{
    static const FName Name(TEXT("Kata.RootMotion.Yaw"));
    return Name;
}

TArray<FName> KataFL::GetRootMotionCurveNames()
{
    return { GetRootMotionCurveNameX(), GetRootMotionCurveNameY(), GetRootMotionCurveNameZ(), GetRootMotionCurveNameYaw() };
}

bool KataFL::HasRootMotionCurves(const UAnimSequenceBase& Animation, bool bForceUseRawData)
{
    for (const FName CurveName : GetRootMotionCurveNames())
    {
        if (!Animation.HasCurveData(CurveName, bForceUseRawData))
        {
            return false;
        }
    }
    return true;
}

bool KataFL::EvaluateRootMotionCurves(const UAnimSequenceBase& Animation, double Time, FKataRootMotionCurveValue& OutValue, bool bForceUseRawData)
{
    if (!HasRootMotionCurves(Animation, bForceUseRawData))
    {
        return false;
    }

    const FAnimExtractContext Context(Time);
    OutValue.Translation.X = Animation.EvaluateCurveData(GetRootMotionCurveNameX(), Context, bForceUseRawData);
    OutValue.Translation.Y = Animation.EvaluateCurveData(GetRootMotionCurveNameY(), Context, bForceUseRawData);
    OutValue.Translation.Z = Animation.EvaluateCurveData(GetRootMotionCurveNameZ(), Context, bForceUseRawData);
    OutValue.Yaw = Animation.EvaluateCurveData(GetRootMotionCurveNameYaw(), Context, bForceUseRawData);
    return true;
}

FTransform KataFL::MakeRootMotionCurveDelta(const FKataRootMotionCurveValue& Start, const FKataRootMotionCurveValue& End)
{
    // 누적값은 첫 프레임 루트 기준이므로, 이동량을 시작 시각 루트의 방향으로 되돌려야
    // ExtractRootMotionFromRange(Start, End)와 같은 국소 변화량이 된다.
    const FQuat StartRotation(FRotator(0.0, Start.Yaw, 0.0));
    const FVector LocalTranslation = StartRotation.UnrotateVector(End.Translation - Start.Translation);
    const FQuat DeltaRotation(FRotator(0.0, End.Yaw - Start.Yaw, 0.0));
    return FTransform(DeltaRotation, LocalTranslation);
}
