#include "UI/KataHUDViewModelResolver.h"

#include "Blueprint/UserWidget.h"
#include "Engine/LocalPlayer.h"
#include "UI/KataHUDSubsystem.h"

UObject* UKataHUDViewModelResolver::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
    const ULocalPlayer* LocalPlayer = UserWidget ? UserWidget->GetOwningLocalPlayer() : nullptr;
    UKataHUDSubsystem* Subsystem = LocalPlayer ? LocalPlayer->GetSubsystem<UKataHUDSubsystem>() : nullptr;
    return Subsystem ? Subsystem->FindOrAddViewModel(ExpectedType) : nullptr;
}

void UKataHUDViewModelResolver::DestroyInstance(UObject* ViewModel, const UMVVMView* View) const
{
}
