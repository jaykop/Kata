#include "UI/KataEnemyHealthBarLayer.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "UI/KataEnemyHealthBarWidget.h"
#include "UI/KataHUDSubsystem.h"
#include "UI/KataVM_EnemyHealth.h"
#include "UI/KataVM_EnemyHealthList.h"

void UKataEnemyHealthBarLayer::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
    const UKataHUDSubsystem* HUDSubsystem = LocalPlayer != nullptr ? LocalPlayer->GetSubsystem<UKataHUDSubsystem>() : nullptr;
    const UKataVM_EnemyHealthList* List = HUDSubsystem != nullptr ? HUDSubsystem->GetEnemyHealthList() : nullptr;
    if (List == nullptr || BarCanvas == nullptr || EntryWidgetClass == nullptr)
    {
        return;
    }

    SyncEntries(*List);
    for (const TPair<TObjectPtr<UKataVM_EnemyHealth>, TObjectPtr<UKataEnemyHealthBarWidget>>& Pair : ActiveWidgets)
    {
        if (Pair.Key != nullptr && Pair.Value != nullptr)
        {
            UpdateEntryPosition(*Pair.Value, *Pair.Key);
        }
    }
}

void UKataEnemyHealthBarLayer::SyncEntries(const UKataVM_EnemyHealthList& List)
{
    const TArray<TObjectPtr<UKataVM_EnemyHealth>>& Entries = List.GetEntries();

    // 목록에서 빠진 항목의 위젯을 숨겨 풀로 돌린다.
    for (auto It = ActiveWidgets.CreateIterator(); It; ++It)
    {
        if (It->Key == nullptr || !Entries.Contains(It->Key))
        {
            if (UKataEnemyHealthBarWidget* Widget = It->Value)
            {
                Widget->SetVisibility(ESlateVisibility::Collapsed);
                PooledWidgets.Add(Widget);
            }
            It.RemoveCurrent();
        }
    }

    for (UKataVM_EnemyHealth* Entry : Entries)
    {
        if (Entry == nullptr || ActiveWidgets.Contains(Entry))
        {
            continue;
        }
        if (UKataEnemyHealthBarWidget* Widget = AcquireWidget())
        {
            Widget->SetEnemyViewModel(Entry);
            ActiveWidgets.Add(Entry, Widget);
        }
    }
}

UKataEnemyHealthBarWidget* UKataEnemyHealthBarLayer::AcquireWidget()
{
    if (PooledWidgets.Num() > 0)
    {
        return PooledWidgets.Pop();
    }

    UKataEnemyHealthBarWidget* Widget = CreateWidget<UKataEnemyHealthBarWidget>(this, EntryWidgetClass);
    if (Widget == nullptr)
    {
        return nullptr;
    }
    if (UCanvasPanelSlot* CanvasSlot = BarCanvas->AddChildToCanvas(Widget))
    {
        // 위치는 바의 아래 가운데를 기준으로 잡아 머리 위에 떠 있게 한다.
        CanvasSlot->SetAutoSize(true);
        CanvasSlot->SetAlignment(FVector2D(0.5, 1.0));
    }
    Widget->SetVisibility(ESlateVisibility::Collapsed);
    return Widget;
}

void UKataEnemyHealthBarLayer::UpdateEntryPosition(UKataEnemyHealthBarWidget& Widget, const UKataVM_EnemyHealth& ViewModel) const
{
    APlayerController* Controller = GetOwningPlayer();
    FVector WorldLocation;
    FVector2D PixelPosition;
    int32 Width = 0;
    int32 Height = 0;
    if (Controller != nullptr)
    {
        Controller->GetViewportSize(Width, Height);
    }
    // ProjectWorldLocationToScreen은 카메라 뒤 지점이면 false를 반환한다.
    const bool bVisible = Controller != nullptr
        && ViewModel.GetBarWorldLocation(WorldLocation)
        && Controller->ProjectWorldLocationToScreen(WorldLocation, PixelPosition, true)
        && PixelPosition.X >= 0.0 && PixelPosition.Y >= 0.0 && PixelPosition.X <= Width && PixelPosition.Y <= Height;
    if (!bVisible)
    {
        Widget.SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Widget.Slot))
    {
        const float Scale = UWidgetLayoutLibrary::GetViewportScale(this);
        CanvasSlot->SetPosition(PixelPosition / FMath::Max(Scale, UE_SMALL_NUMBER));
    }
    Widget.SetVisibility(ESlateVisibility::HitTestInvisible);
}
