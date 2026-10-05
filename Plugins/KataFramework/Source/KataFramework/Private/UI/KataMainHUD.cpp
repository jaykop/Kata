#include "UI/KataMainHUD.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "GameFramework/PlayerController.h"
#include "Rendering/DrawElements.h"
#include "Targeting/KataTargetPointComponent.h"

UKataMainHUD::UKataMainHUD(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // 지점과 카메라는 매 프레임 움직이므로 기본 마커의 Paint 결과를 캐시하지 않는다.
    ForceVolatile(true);
}

void UKataMainHUD::SetLockPoint(UKataTargetPointComponent* Point)
{
    UKataTargetPointComponent* OldPoint = LockPoint.Get();
    if (OldPoint != Point)
    {
        LockPoint = Point;
        OnLockPointChanged(OldPoint, Point);
    }
}

UKataTargetPointComponent* UKataMainHUD::GetLockPoint() const
{
    return LockPoint.Get();
}

bool UKataMainHUD::GetLockMarkerPosition(FVector2D& OutPosition) const
{
    OutPosition = FVector2D::ZeroVector;
    const UKataTargetPointComponent* Point = LockPoint.Get();
    APlayerController* Controller = GetOwningPlayer();
    if (!IsValid(Point) || !Point->IsRegistered() || !Point->IsTargetPointEnabled() || Controller == nullptr)
    {
        return false;
    }
    FVector2D PixelPosition;
    if (!Controller->ProjectWorldLocationToScreen(Point->GetComponentLocation(), PixelPosition, true))
    {
        return false;
    }
    int32 Width = 0;
    int32 Height = 0;
    Controller->GetViewportSize(Width, Height);
    if (PixelPosition.X < 0.0 || PixelPosition.Y < 0.0 || PixelPosition.X > Width || PixelPosition.Y > Height)
    {
        return false;
    }
    const float Scale = UWidgetLayoutLibrary::GetViewportScale(this);
    OutPosition = PixelPosition / FMath::Max(Scale, UE_SMALL_NUMBER);
    return true;
}

int32 UKataMainHUD::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
    FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
    const int32 BaseLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
    FVector2D Position;
    if (!bDrawDefaultLockMarker || !GetLockMarkerPosition(Position))
    {
        return BaseLayer;
    }
    // 도형은 HUD의 로컬 공간에서 그린다. 텍스처 없이도 락온 연결을 확인할 수 있는 기본 표시다.
    const float Radius = FMath::Max(LockMarkerRadius, 1.0f);
    TArray<FVector2D> Points;
    Points.Add(Position + FVector2D(0.0, -Radius));
    Points.Add(Position + FVector2D(Radius, 0.0));
    Points.Add(Position + FVector2D(0.0, Radius));
    Points.Add(Position + FVector2D(-Radius, 0.0));
    // TArray 내부 원소를 같은 배열의 Add에 참조로 넘기면 주소 검사에 걸리므로 좌표를 다시 계산한다.
    Points.Add(Position + FVector2D(0.0, -Radius));
    FSlateDrawElement::MakeLines(OutDrawElements, BaseLayer + 1, AllottedGeometry.ToPaintGeometry(), Points,
        ESlateDrawEffect::None, LockMarkerColor * InWidgetStyle.GetColorAndOpacityTint(), true, 2.0f);
    return BaseLayer + 1;
}
