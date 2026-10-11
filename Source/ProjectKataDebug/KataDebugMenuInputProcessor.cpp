#include "KataDebugMenuInputProcessor.h"

#include "InputCoreTypes.h"
#include "Input/Events.h"
#include "KataDebugMenuSubsystem.h"

FKataDebugMenuInputProcessor::FKataDebugMenuInputProcessor(UKataDebugMenuSubsystem* InOwner)
    : Owner(InOwner)
{
}

void FKataDebugMenuInputProcessor::Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor)
{
}

bool FKataDebugMenuInputProcessor::HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent)
{
    if (InKeyEvent.GetKey() != EKeys::Home || InKeyEvent.IsRepeat())
    {
        return false;
    }

    UKataDebugMenuSubsystem* Menu = Owner.Get();
    if (Menu == nullptr || !Menu->IsViewportFocused())
    {
        return false;
    }

    Menu->ToggleMenu();
    return true;
}
