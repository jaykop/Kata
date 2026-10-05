#include "KataLockOnData.h"

#include "Curves/CurveFloat.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "KataLockOnData"

#if WITH_EDITOR
EDataValidationResult UKataLockOnData::IsDataValid(FDataValidationContext& Context) const
{
    const EDataValidationResult Result = Super::IsDataValid(Context);
    ValidateBlendCurve(Settings.BlendInCurve, Context);
    return Result;
}

void UKataLockOnData::ValidateBlendCurve(const UCurveFloat* Curve, FDataValidationContext& Context)
{
    if (Curve == nullptr)
    {
        return;
    }

    // 카메라 블렌드는 단조 증가 곡선만 허용한다. 되돌아가는 곡선은 블렌드 중 Pitch가 목표를 넘거나 되돌아가게 만든다.
    constexpr int32 SampleCount = 32;
    float Previous = Curve->GetFloatValue(0.0f);
    for (int32 Index = 1; Index <= SampleCount; ++Index)
    {
        const float Value = Curve->GetFloatValue(static_cast<float>(Index) / SampleCount);
        if (Value < Previous - UE_KINDA_SMALL_NUMBER)
        {
            Context.AddWarning(FText::Format(LOCTEXT("NonMonotonicBlendCurve",
                "Blend curve '{0}' must be non-decreasing on [0, 1]. Camera pitch can overshoot during the lock-on blend."),
                FText::FromString(Curve->GetName())));
            break;
        }
        Previous = Value;
    }

    if (!FMath::IsNearlyEqual(Curve->GetFloatValue(0.0f), 0.0f, 0.01f) || !FMath::IsNearlyEqual(Curve->GetFloatValue(1.0f), 1.0f, 0.01f))
    {
        Context.AddWarning(FText::Format(LOCTEXT("BlendCurveRange",
            "Blend curve '{0}' should map 0 to 0 and 1 to 1. Values are clamped to [0, 1] and the blend snaps to the target when it ends."),
            FText::FromString(Curve->GetName())));
    }
}
#endif

#undef LOCTEXT_NAMESPACE
