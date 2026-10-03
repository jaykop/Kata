#include "Data/KataRowId.h"

#include "Data/KataDataCollection.h"
#include "Data/KataDataSettings.h"
#include "KataFrameworkLog.h"

const FKataCharacterRow* FKataCharacterId::Find(const UDataTable** OutTable) const
{
    if (OutTable != nullptr)
    {
        *OutTable = nullptr;
    }
    if (!IsValid())
    {
        return nullptr;
    }

    const UKataDataCollection* Collection = UKataDataSettings::Get()->GetDataCollection();
    if (Collection == nullptr)
    {
        UE_LOG(LogKataFramework, Warning, TEXT("Kata Data: cannot find character %s because no data collection is set in Project Settings > Game > Kata Data."),
            *ToString());
        return nullptr;
    }
    return Collection->FindCharacterRow(RowName, OutTable);
}
