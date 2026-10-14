#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "KataEnemyHealthBarWidget.generated.h"

class UKataVM_EnemyHealth;

/**
 * 적 체력바 항목 위젯의 기반. Widget Blueprint에서 UKataVM_EnemyHealth를 Manual 생성 방식으로 추가하고 비율 등을 바인딩한다.
 * UKataEnemyHealthBarLayer가 위젯을 풀에서 꺼낼 때 SetEnemyViewModel로 그 항목의 ViewModel을 넣는다.
 */
UCLASS(Abstract, meta = (DisplayName = "Kata Enemy Health Bar Widget"))
class KATAFRAMEWORK_API UKataEnemyHealthBarWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /**
     * 이 위젯의 MVVM View에 ViewModel을 넣는다. 같은 ViewModel이면 다시 넣지 않는다.
     * Widget Blueprint에 UKataVM_EnemyHealth가 Manual로 선언되지 않았으면 넣지 못하고 경고 로그를 남긴다.
     */
    void SetEnemyViewModel(UKataVM_EnemyHealth* ViewModel);

    UKataVM_EnemyHealth* GetEnemyViewModel() const { return EnemyViewModel.Get(); }

private:
    TWeakObjectPtr<UKataVM_EnemyHealth> EnemyViewModel;
};
