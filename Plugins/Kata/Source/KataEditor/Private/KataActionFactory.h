#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "KataActionFactory.generated.h"

class UKataAction;

/** 새 Kata와 부모를 참조하는 자식 Kata를 생성한다. */
UCLASS()
class UKataActionFactory : public UFactory
{
    GENERATED_BODY()
public:
    UKataActionFactory();
    virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name,
        EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;

    UPROPERTY()
    TObjectPtr<UKataAction> ParentAction;
};
