#include "Animation/KataFL_RootMotionCurveEditor.h"

#include "Animation/AnimData/CurveIdentifier.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "Animation/AnimData/IAnimationDataModel.h"
#include "Animation/AnimSequenceBase.h"
#include "Curves/RichCurve.h"

DEFINE_LOG_CATEGORY(LogKataRootMotionCurve);

double KataFL::UnwrapRootMotionYaw(double RawYaw, double PreviousYaw)
{
    return PreviousYaw + FMath::UnwindDegrees(RawYaw - PreviousYaw);
}

FKataRootMotionCurveValue KataFL::MakeRootMotionCurveValue(const FTransform& RootFromStart, const FKataRootMotionCurveValue* Previous)
{
    const double RawYaw = RootFromStart.Rotator().Yaw;

    FKataRootMotionCurveValue Value;
    Value.Translation = RootFromStart.GetTranslation();
    Value.Yaw = Previous != nullptr ? UnwrapRootMotionYaw(RawYaw, Previous->Yaw) : RawYaw;
    return Value;
}

bool KataFL::HasAnyRootMotionCurveInModel(const UAnimSequenceBase& Animation)
{
    const IAnimationDataModel* Model = Animation.GetDataModel();
    if (Model == nullptr)
    {
        return false;
    }
    for (const FName CurveName : GetRootMotionCurveNames())
    {
        if (Model->FindFloatCurve(FAnimationCurveIdentifier(CurveName, ERawCurveTrackTypes::RCT_Float)) != nullptr)
        {
            return true;
        }
    }
    return false;
}

void KataFL::WriteRootMotionCurves(UAnimSequenceBase& Animation, TConstArrayView<double> Times, TConstArrayView<FKataRootMotionCurveValue> Values,
    const FText& Description)
{
    if (Times.Num() != Values.Num() || Times.IsEmpty())
    {
        return;
    }

    TArray<FRichCurveKey> CurveKeys[4];
    for (int32 Index = 0; Index < Times.Num(); ++Index)
    {
        const float Time = static_cast<float>(Times[Index]);
        const FKataRootMotionCurveValue& Value = Values[Index];
        CurveKeys[0].Add(FRichCurveKey(Time, static_cast<float>(Value.Translation.X)));
        CurveKeys[1].Add(FRichCurveKey(Time, static_cast<float>(Value.Translation.Y)));
        CurveKeys[2].Add(FRichCurveKey(Time, static_cast<float>(Value.Translation.Z)));
        CurveKeys[3].Add(FRichCurveKey(Time, static_cast<float>(Value.Yaw)));
    }

    const TArray<FName> CurveNames = GetRootMotionCurveNames();
    IAnimationDataController& Controller = Animation.GetController();
    IAnimationDataController::FScopedBracket Bracket(Controller, Description);
    for (int32 CurveIndex = 0; CurveIndex < CurveNames.Num(); ++CurveIndex)
    {
        const FAnimationCurveIdentifier CurveId(CurveNames[CurveIndex], ERawCurveTrackTypes::RCT_Float);
        if (Animation.GetDataModel()->FindFloatCurve(CurveId) == nullptr)
        {
            Controller.AddCurve(CurveId);
        }
        Controller.SetCurveKeys(CurveId, CurveKeys[CurveIndex]);
    }
}
