#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "KataGraphFactory.generated.h"

/** Content Browser에서 새 Kata Graph 에셋을 만든다. */
UCLASS()
class UKataGraphFactory : public UFactory
{
    GENERATED_BODY()

public:
    UKataGraphFactory();

    virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name,
        EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};
