#pragma once

#include "CoreMinimal.h"
#include "AutoLayout/KataGraphLayoutStrategy.h"
#include "KataGraphTreeLayoutStrategy.generated.h"

UCLASS()
class KATAGRAPHEDITOR_API UKataGraphTreeLayoutStrategy : public UKataGraphLayoutStrategy
{
	GENERATED_BODY()
public:
	UKataGraphTreeLayoutStrategy();
	virtual ~UKataGraphTreeLayoutStrategy();

	virtual void Layout(UEdGraph* EdGraph) override;

protected:
	void InitPass(UKataGraphNodeBase* RootNode, const FVector2D& Anchor);
	bool ResolveConflictPass(UKataGraphNodeBase* Node);

	bool ResolveConflict(UKataGraphNodeBase* LRoot, UKataGraphNodeBase* RRoot);

	void GetLeftContour(UKataGraphNodeBase* RootNode, int32 Level, TArray<UKataEdNode*>& Contour);
	void GetRightContour(UKataGraphNodeBase* RootNode, int32 Level, TArray<UKataEdNode*>& Contour);
	
	void ShiftSubTree(UKataGraphNodeBase* RootNode, const FVector2D& Offset);

	void UpdateParentNodePosition(UKataGraphNodeBase* RootNode);
};
