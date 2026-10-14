#include "UI/KataVM_EnemyHealthList.h"

#include "UI/KataVM_EnemyHealth.h"

void UKataVM_EnemyHealthList::AddEntry(UKataVM_EnemyHealth* Entry)
{
    if (Entry != nullptr && !Entries.Contains(Entry))
    {
        Entries.Add(Entry);
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Entries);
    }
}

void UKataVM_EnemyHealthList::RemoveEntry(UKataVM_EnemyHealth* Entry)
{
    if (Entries.Remove(Entry) > 0)
    {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Entries);
    }
}

void UKataVM_EnemyHealthList::ClearEntries()
{
    if (Entries.Num() > 0)
    {
        Entries.Reset();
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Entries);
    }
}
