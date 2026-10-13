#include "UI/KataHUDSubsystem.h"

#include "INotifyFieldValueChanged.h"
#include "KataFrameworkLog.h"

void UKataHUDSubsystem::Deinitialize()
{
    ViewModels.Reset();
    Super::Deinitialize();
}

UObject* UKataHUDSubsystem::FindOrAddViewModel(const UClass* ViewModelClass)
{
    if (!ViewModelClass)
    {
        return nullptr;
    }

    for (UObject* ViewModel : ViewModels)
    {
        // 정확히 같은 클래스만 재사용한다. 파생 클래스를 요청한 바인딩에 부모 인스턴스를 넘기지 않기 위해서다.
        if (ViewModel && ViewModel->GetClass() == ViewModelClass)
        {
            return ViewModel;
        }
    }

    if (!ViewModelClass->ImplementsInterface(UNotifyFieldValueChanged::StaticClass())
        || ViewModelClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
    {
        UE_LOG(LogKataFramework, Warning, TEXT("UKataHUDSubsystem: '%s' cannot be used as a HUD viewmodel."), *ViewModelClass->GetPathName());
        return nullptr;
    }

    UObject* NewViewModel = NewObject<UObject>(this, const_cast<UClass*>(ViewModelClass));
    ViewModels.Add(NewViewModel);
    return NewViewModel;
}
