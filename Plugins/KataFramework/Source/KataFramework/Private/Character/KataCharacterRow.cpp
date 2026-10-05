#include "Character/KataCharacterRow.h"

#include "Animation/AnimInstance.h"
#include "Animation/KataAnimLayerSetup.h"
#include "Character/KataCharacter.h"
#include "Controller/KataAIController.h"
#include "Data/KataAIData.h"
#include "Engine/SkeletalMesh.h"
#include "Equipment/KataEquipmentSetup.h"
#include "Input/KataInputConfig.h"
#include "KataGraph.h"
#include "StateTree.h"

void FKataCharacterRow::GatherAssetsToLoad(TArray<FSoftObjectPath>& OutPaths) const
{
    if (!CharacterClass.IsNull())
    {
        OutPaths.Add(CharacterClass.ToSoftObjectPath());
    }
    if (!SkeletalMesh.IsNull())
    {
        OutPaths.Add(SkeletalMesh.ToSoftObjectPath());
    }
    if (!AnimClass.IsNull())
    {
        OutPaths.Add(AnimClass.ToSoftObjectPath());
    }
    if (!EquipmentSetup.IsNull())
    {
        OutPaths.Add(EquipmentSetup.ToSoftObjectPath());
    }
    if (!AnimLayerSetup.IsNull())
    {
        OutPaths.Add(AnimLayerSetup.ToSoftObjectPath());
    }
}

void FKataPlayerCharacterRow::GatherAssetsToLoad(TArray<FSoftObjectPath>& OutPaths) const
{
    FKataCharacterRow::GatherAssetsToLoad(OutPaths);

    if (!InputConfig.IsNull())
    {
        OutPaths.Add(InputConfig.ToSoftObjectPath());
    }
    if (!Graph.IsNull())
    {
        OutPaths.Add(Graph.ToSoftObjectPath());
    }
}

void FKataNPCCharacterRow::GatherAssetsToLoad(TArray<FSoftObjectPath>& OutPaths) const
{
    FKataCharacterRow::GatherAssetsToLoad(OutPaths);
    if (!AIControllerClass.IsNull())
    {
        OutPaths.Add(AIControllerClass.ToSoftObjectPath());
    }
    if (!AIData.IsNull())
    {
        OutPaths.Add(AIData.ToSoftObjectPath());
    }
}
