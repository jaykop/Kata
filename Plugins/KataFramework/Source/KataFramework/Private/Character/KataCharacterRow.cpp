#include "Character/KataCharacterRow.h"

#include "Animation/AnimInstance.h"
#include "Character/KataCharacter.h"
#include "Engine/SkeletalMesh.h"
#include "Input/KataInputConfig.h"
#include "KataGraph.h"

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
