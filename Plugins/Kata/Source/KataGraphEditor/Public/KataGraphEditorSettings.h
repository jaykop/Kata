#pragma once

#include "CoreMinimal.h"
#include "KataGraphEditorSettings.generated.h"

UENUM(BlueprintType)
enum class EKataGraphLayoutStrategy : uint8
{
	Tree,
	ForceDirected,
};

UCLASS()
class KATAGRAPHEDITOR_API UKataGraphEditorSettings : public UObject
{
	GENERATED_BODY()

public:
	UKataGraphEditorSettings();
	virtual ~UKataGraphEditorSettings();

	UPROPERTY(EditDefaultsOnly, Category = "AutoArrange")
	float OptimalDistance;

	UPROPERTY(EditDefaultsOnly, AdvancedDisplay, Category = "AutoArrange")
	EKataGraphLayoutStrategy AutoLayoutStrategy;

	UPROPERTY(EditDefaultsOnly, AdvancedDisplay, Category = "AutoArrange")
	int32 MaxIteration;

	UPROPERTY(EditDefaultsOnly, AdvancedDisplay, Category = "AutoArrange")
	bool bFirstPassOnly;

	UPROPERTY(EditDefaultsOnly, AdvancedDisplay, Category = "AutoArrange")
	bool bRandomInit;

	UPROPERTY(EditDefaultsOnly, AdvancedDisplay, Category = "AutoArrange")
	float InitTemperature;

	UPROPERTY(EditDefaultsOnly, AdvancedDisplay, Category = "AutoArrange")
	float CoolDownRate;
};
