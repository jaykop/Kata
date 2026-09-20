#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "KataAssetFactory.generated.h"

class UKataAsset;

/** 새 Kata와 부모를 참조하는 자식 Kata를 생성한다. */
UCLASS()
class UKataAssetFactory : public UFactory
{
    GENERATED_BODY()
public:
    UKataAssetFactory();
    virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name,
        EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;

    UPROPERTY()
    TObjectPtr<UKataAsset> ParentAsset;
};
