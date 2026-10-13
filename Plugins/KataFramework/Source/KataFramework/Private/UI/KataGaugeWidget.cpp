#include "UI/KataGaugeWidget.h"

#include "Components/ProgressBar.h"

void UKataGaugeWidget::SetPercent(float InPercent)
{
    const float NewPercent = FMath::Clamp(InPercent, 0.0f, 1.0f);
    const bool bDelayDecrease = bHasValue && bUseDelayedBar && DelayedBar != nullptr;
    if (!bDelayDecrease || NewPercent >= DelayedPercent)
    {
        DelayedPercent = NewPercent;
        HoldRemaining = 0.0f;
    }
    else if (NewPercent < Percent)
    {
        HoldRemaining = DelayedBarHoldTime;
    }
    // 지연 바가 줄어드는 도중에 회복해도 DelayedPercent보다 낮으면 지연 바는 계속 줄어든다.

    Percent = NewPercent;
    bHasValue = true;
    ApplyBars();
}

void UKataGaugeWidget::NativePreConstruct()
{
    Super::NativePreConstruct();
    if (FillBar != nullptr)
    {
        FillBar->SetFillColorAndOpacity(FillColor);
    }
    ApplyBars();
}

void UKataGaugeWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    if (DelayedPercent <= Percent)
    {
        return;
    }

    if (HoldRemaining > 0.0f)
    {
        HoldRemaining -= InDeltaTime;
        return;
    }

    DelayedPercent = FMath::Max(Percent, DelayedPercent - DelayedBarDrainRate * InDeltaTime);
    ApplyBars();
}

void UKataGaugeWidget::ApplyBars()
{
    if (FillBar != nullptr)
    {
        FillBar->SetPercent(Percent);
    }
    if (DelayedBar != nullptr)
    {
        DelayedBar->SetPercent(DelayedPercent);
    }
}
