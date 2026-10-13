#pragma once

#include "CoreMinimal.h"
#include "View/MVVMViewModelContextResolver.h"
#include "KataHUDViewModelResolver.generated.h"

/**
 * Widget Blueprint의 ViewModel 생성 방식을 Resolver로 둘 때 지정한다.
 * 위젯을 소유한 로컬 플레이어의 UKataHUDSubsystem에서 요청 클래스의 ViewModel을 찾거나 만든다.
 * 소유 로컬 플레이어가 없으면(디자이너 미리보기 등) nullptr을 반환하므로 바인딩은 값을 받지 못한다.
 */
UCLASS(EditInlineNew, meta = (DisplayName = "Kata HUD Viewmodel Resolver"))
class KATAFRAMEWORK_API UKataHUDViewModelResolver : public UMVVMViewModelContextResolver
{
    GENERATED_BODY()

public:
    virtual UObject* CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const override;

    /** ViewModel은 서브시스템이 소유하므로 위젯이 사라져도 파괴하지 않는다. */
    virtual void DestroyInstance(UObject* ViewModel, const UMVVMView* View) const override;
};
