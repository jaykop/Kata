#include "UI/KataEnemyHealthBarWidget.h"

#include "KataFrameworkLog.h"
#include "MVVMSubsystem.h"
#include "UI/KataVM_EnemyHealth.h"
#include "View/MVVMView.h"

void UKataEnemyHealthBarWidget::SetEnemyViewModel(UKataVM_EnemyHealth* ViewModel)
{
    if (ViewModel == nullptr || EnemyViewModel.Get() == ViewModel)
    {
        return;
    }
    UMVVMView* View = UMVVMSubsystem::GetViewFromUserWidget(this);
    if (View == nullptr || !View->SetViewModelByClass(ViewModel))
    {
        UE_LOG(LogKataFramework, Warning, TEXT("'%s' could not receive an enemy health viewmodel. Add Kata Enemy Health Viewmodel with the Manual creation type."),
            *GetClass()->GetName());
        return;
    }
    EnemyViewModel = ViewModel;
}
