#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "KataMainHUD.generated.h"

class UKataTargetPointComponent;

/** 플레이어 HUD의 기반. 기본 락온 마커를 제공하고 Blueprint 파생에서 추가 UI를 구성할 수 있다. */
UCLASS(Blueprintable, meta = (DisplayName = "Kata Main HUD"))
class KATAFRAMEWORK_API UKataMainHUD : public UUserWidget
{
    GENERATED_BODY()

public:
    UKataMainHUD(const FObjectInitializer& ObjectInitializer);

    /** 컨트롤러가 락온 변경을 전달한다. 지점은 약한 참조로 보관하며 nullptr이면 표시를 해제한다. */
    void SetLockPoint(UKataTargetPointComponent* Point);

    UFUNCTION(BlueprintPure, Category = "Kata|HUD")
    UKataTargetPointComponent* GetLockPoint() const;

    /** 뷰포트 상대 좌표를 DPI 보정한 위젯 좌표로 돌려준다. 지점이 없거나 화면 밖이면 false다. */
    UFUNCTION(BlueprintPure, Category = "Kata|HUD")
    bool GetLockMarkerPosition(FVector2D& OutPosition) const;

protected:
    virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
        FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

    /** 커스텀 마커를 만들면 기본 도형을 끈다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|HUD")
    bool bDrawDefaultLockMarker = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|HUD", meta = (ClampMin = "1.0"))
    float LockMarkerRadius = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|HUD")
    FLinearColor LockMarkerColor = FLinearColor::White;

    /** 포인트가 바뀔 때만 호출한다. 이동에 따른 화면 위치는 GetLockMarkerPosition으로 읽는다. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Kata|HUD")
    void OnLockPointChanged(UKataTargetPointComponent* OldPoint, UKataTargetPointComponent* NewPoint);

private:
    TWeakObjectPtr<UKataTargetPointComponent> LockPoint;
};
