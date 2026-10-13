#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "KataGaugeWidget.generated.h"

class UProgressBar;

/**
 * 비율 하나를 막대로 그리는 게이지 위젯의 기반. Widget Blueprint에서 FillBar(필수)와 DelayedBar(선택) Progress Bar를 배치한다.
 * ViewModel의 비율을 SetPercent에 바인딩한다.
 * 지연 바를 쓰면 값이 줄 때 DelayedBar가 대기 시간 동안 이전 값에 머문 뒤 FillBar까지 줄어든다.
 * 회복값이 현재 DelayedBar 값 이상이면 두 막대를 즉시 맞춘다. 그보다 낮으면 기존 대기·감소 처리를 이어 간다.
 * 지연 바 진행은 위젯 Tick의 경과 시간을 쓰므로 게임 시간 배율의 영향을 받지 않는다.
 */
UCLASS(Abstract, meta = (DisplayName = "Kata Gauge Widget"))
class KATAFRAMEWORK_API UKataGaugeWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** 표시할 비율을 정한다. 0~1로 제한한다. 처음 받은 값은 지연 없이 두 막대에 바로 반영한다. */
    UFUNCTION(BlueprintCallable, Category = "Kata|HUD")
    void SetPercent(float InPercent);

    UFUNCTION(BlueprintPure, Category = "Kata|HUD")
    float GetPercent() const { return Percent; }

protected:
    virtual void NativePreConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    UPROPERTY(BlueprintReadOnly, Category = "Kata|HUD", meta = (BindWidget))
    TObjectPtr<UProgressBar> FillBar;

    /** FillBar 뒤에 겹쳐 둔다. 없으면 지연 바 없이 FillBar만 갱신한다. */
    UPROPERTY(BlueprintReadOnly, Category = "Kata|HUD", meta = (BindWidgetOptional))
    TObjectPtr<UProgressBar> DelayedBar;

    /** FillBar의 채움 색. 같은 게이지 Widget Blueprint를 HP·스태미나·마나처럼 여러 용도로 쓸 때 인스턴스마다 정한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|HUD")
    FLinearColor FillColor = FLinearColor(0.75f, 0.08f, 0.08f);

    /** 끄면 값이 줄 때도 DelayedBar가 FillBar와 함께 즉시 줄어든다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|HUD")
    bool bUseDelayedBar = true;

    /** 값이 줄어든 뒤 DelayedBar가 줄기 시작할 때까지 기다리는 시간(초). 대기 중 다시 줄면 처음부터 다시 기다린다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|HUD", meta = (ClampMin = "0.0", Units = "s", EditCondition = "bUseDelayedBar"))
    float DelayedBarHoldTime = 0.5f;

    /** DelayedBar가 초당 줄어드는 비율. 1이면 가득 찬 막대가 1초에 빈다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|HUD", meta = (ClampMin = "0.01", EditCondition = "bUseDelayedBar"))
    float DelayedBarDrainRate = 0.5f;

private:
    void ApplyBars();

    float Percent = 1.0f;
    float DelayedPercent = 1.0f;
    float HoldRemaining = 0.0f;
    bool bHasValue = false;
};
