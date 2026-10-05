#include "FunctionLibraries/KataFL_ActionGroup.h"

bool UKataFL_ActionGroup::TrySelectEntry(const UKataActionGroup* Group, const TArray<int32>& CandidateIndices,
    double RandomValue, FKataActionGroupSelection& OutSelection)
{
    OutSelection = FKataActionGroupSelection();
    if (!IsValid(Group) || !FMath::IsFinite(RandomValue) || RandomValue < 0.0 || RandomValue >= 1.0)
    {
        return false;
    }

    TArray<int32> Candidates;
    TSet<int32> Seen;
    double TotalWeight = 0.0;
    for (const int32 Index : CandidateIndices)
    {
        if (!Group->Entries.IsValidIndex(Index) || Seen.Contains(Index))
        {
            continue;
        }
        Seen.Add(Index);
        const FKataActionGroupEntry& Entry = Group->Entries[Index];
        if (!Entry.HasValidAsset() || !Entry.HasValidPayload() || !FMath::IsFinite(Entry.Weight) || Entry.Weight <= 0.0f)
        {
            continue;
        }
        Candidates.Add(Index);
        TotalWeight += static_cast<double>(Entry.Weight);
    }
    if (Candidates.IsEmpty())
    {
        return false;
    }

    const double Threshold = RandomValue * TotalWeight;
    double AccumulatedWeight = 0.0;
    // 경계의 반올림 때문에 선택이 실패하지 않도록 마지막 유효 후보를 기본값으로 둔다.
    int32 SelectedIndex = Candidates.Last();
    for (const int32 Index : Candidates)
    {
        AccumulatedWeight += static_cast<double>(Group->Entries[Index].Weight);
        if (Threshold < AccumulatedWeight)
        {
            SelectedIndex = Index;
            break;
        }
    }
    OutSelection.EntryIndex = SelectedIndex;
    OutSelection.Entry = Group->Entries[SelectedIndex];
    return true;
}
