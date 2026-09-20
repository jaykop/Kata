#pragma once

#include "CoreMinimal.h"
#include "AutoLayout/KataGraphLayoutStrategy.h"
#include "KataGraphForceDirectedLayoutStrategy.generated.h"

UCLASS()
class KATAGRAPHEDITOR_API UKataGraphForceDirectedLayoutStrategy : public UKataGraphLayoutStrategy
{
	GENERATED_BODY()
public:
	UKataGraphForceDirectedLayoutStrategy();
	virtual ~UKataGraphForceDirectedLayoutStrategy();

	virtual void Layout(UEdGraph* EdGraph) override;

protected:
	virtual FBox2D LayoutOneTree(UKataGraphNodeBase* RootNode, const FBox2D& PreTreeBound);

protected:
	bool bRandomInit;
	float InitTemperature;
	float CoolDownRate;
};
