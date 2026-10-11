#pragma once

#include "CoreMinimal.h"
#include "Framework/Application/IInputProcessor.h"
#include "UObject/WeakObjectPtrTemplates.h"

class UKataDebugMenuSubsystem;

/**
 * 디버그 메뉴의 단축키(Home)를 받는 Slate 입력 전처리기다. 게임 입력 에셋과 섞이지 않도록 Slate 단계에서 키를 본다.
 * 자기 메뉴의 게임 뷰포트가 포커스를 가졌을 때만 키를 소비하므로 에디터 창에서 누른 Home은 그대로 지나간다.
 */
class FKataDebugMenuInputProcessor : public IInputProcessor
{
public:
    explicit FKataDebugMenuInputProcessor(UKataDebugMenuSubsystem* InOwner);

    //~ Begin IInputProcessor Interface
    virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override;
    virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override;
    virtual const TCHAR* GetDebugName() const override { return TEXT("KataDebugMenu"); }
    //~ End IInputProcessor Interface

private:
    TWeakObjectPtr<UKataDebugMenuSubsystem> Owner;
};
