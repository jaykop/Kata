#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "KataActionFactory.generated.h"

class UKataAction;
class UKataActionTemplate;

/** 새 Kata와 부모 Template을 참조하는 자식 Kata를 생성한다. */
UCLASS()
class UKataActionFactory : public UFactory
{
    GENERATED_BODY()
public:
    UKataActionFactory();
    virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name,
        EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;

    /**
     * 저장 위치 대화상자를 열어 Template을 부모로 둔 새 Kata를 같은 폴더에 만들고 에디터로 연다.
     * 사용자가 대화상자를 취소하면 nullptr을 반환한다.
     */
    static UKataAction* CreateChildWithDialog(UKataActionTemplate* Template);

    UPROPERTY()
    TObjectPtr<UKataActionTemplate> ParentAction;
};

/** 부모 없이 쓰는 새 Kata Action Template을 생성한다. */
UCLASS()
class UKataActionTemplateFactory : public UFactory
{
    GENERATED_BODY()
public:
    UKataActionTemplateFactory();
    virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name,
        EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};
