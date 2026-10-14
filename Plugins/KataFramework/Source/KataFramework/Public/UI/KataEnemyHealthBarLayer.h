#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "KataEnemyHealthBarLayer.generated.h"

class UCanvasPanel;
class UKataEnemyHealthBarWidget;
class UKataVM_EnemyHealth;
class UKataVM_EnemyHealthList;

/**
 * 적 체력바를 화면 공간에 그리는 레이어. MainHUD 안에 화면 전체를 덮도록 배치한다.
 *
 * Widget Blueprint에 BarCanvas라는 Canvas Panel을 둔다. Tick마다 로컬 플레이어 HUD 서브시스템의 목록 ViewModel에 맞춰
 * 항목 위젯을 꺼내거나 숨기고, 각 대상의 바 위치를 화면에 투영한다. 카메라 뒤나 화면 밖이면 숨긴다.
 * 숨긴 항목 위젯은 지우지 않고 다음 항목에 다시 쓴다. 레이어는 화면 좌상단이 뷰포트 좌상단과 같다고 가정한다.
 */
UCLASS(Abstract, meta = (DisplayName = "Kata Enemy Health Bar Layer"))
class KATAFRAMEWORK_API UKataEnemyHealthBarLayer : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    UPROPERTY(BlueprintReadOnly, Category = "Kata|HUD", meta = (BindWidget))
    TObjectPtr<UCanvasPanel> BarCanvas;

    /** 항목마다 만들 위젯. 비어 있으면 아무것도 그리지 않는다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kata|HUD")
    TSubclassOf<UKataEnemyHealthBarWidget> EntryWidgetClass;

private:
    /** 목록에 맞춰 항목 위젯을 배정·회수한다. */
    void SyncEntries(const UKataVM_EnemyHealthList& List);

    /** 대상 위치를 화면에 투영해 위젯 위치를 정한다. 투영하지 못하면 숨긴다. */
    void UpdateEntryPosition(UKataEnemyHealthBarWidget& Widget, const UKataVM_EnemyHealth& ViewModel) const;

    UKataEnemyHealthBarWidget* AcquireWidget();

    UPROPERTY(Transient)
    TMap<TObjectPtr<UKataVM_EnemyHealth>, TObjectPtr<UKataEnemyHealthBarWidget>> ActiveWidgets;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UKataEnemyHealthBarWidget>> PooledWidgets;
};
