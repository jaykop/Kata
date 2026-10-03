#include "Data/KataDataCollection.h"

#include "Character/KataCharacterRow.h"
#include "Engine/DataTable.h"
#include "KataFrameworkLog.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "KataDataCollection"

const FKataCharacterRow* UKataDataCollection::FindCharacterRow(FName RowName, const UDataTable** OutTable) const
{
    if (OutTable != nullptr)
    {
        *OutTable = nullptr;
    }
    if (RowName.IsNone())
    {
        return nullptr;
    }

    TArray<const UDataTable*> Tables;
    GetCharacterTables(Tables);

    // GetCheckedTable이 두 칸 모두 FKataCharacterRow 계열 행 구조만 통과시키므로 기반 타입으로 해석해도 된다.
    return reinterpret_cast<const FKataCharacterRow*>(FindRowInTables(Tables, RowName, OutTable));
}

void UKataDataCollection::GetCharacterTables(TArray<const UDataTable*>& OutTables) const
{
    OutTables.Reset();
    if (const UDataTable* Table = GetCheckedTable(PlayerCharacterTable, FKataPlayerCharacterRow::StaticStruct()))
    {
        OutTables.Add(Table);
    }
    if (const UDataTable* Table = GetCheckedTable(NPCCharacterTable, FKataNPCCharacterRow::StaticStruct()))
    {
        OutTables.Add(Table);
    }
}

const UDataTable* UKataDataCollection::GetCheckedTable(const UDataTable* Table, const UScriptStruct* RequiredRowStruct) const
{
    if (Table == nullptr)
    {
        return nullptr;
    }

    // 편집 화면은 RequiredAssetDataTags로 행 구조를 거르지만, 지정한 뒤 행 구조를 바꾼 테이블은 걸러지지 않는다.
    const UScriptStruct* RowStruct = Table->GetRowStruct();
    if (RowStruct == nullptr || !RowStruct->IsChildOf(RequiredRowStruct))
    {
        UE_LOG(LogKataFramework, Error, TEXT("Kata Data: data table %s in %s must use row structure %s, but uses %s."),
            *GetNameSafe(Table), *GetNameSafe(this), *GetNameSafe(RequiredRowStruct), *GetNameSafe(RowStruct));
        return nullptr;
    }
    return Table;
}

const uint8* UKataDataCollection::FindRowInTables(TConstArrayView<const UDataTable*> Tables, FName RowName, const UDataTable** OutTable)
{
    const uint8* FoundRow = nullptr;
    const UDataTable* FoundTable = nullptr;
    for (const UDataTable* Table : Tables)
    {
        const uint8* Row = Table->FindRowUnchecked(RowName);
        if (Row == nullptr)
        {
            continue;
        }
        if (FoundRow != nullptr)
        {
            UE_LOG(LogKataFramework, Error, TEXT("Kata Data: row %s exists in both %s and %s. Using the row in %s."),
                *RowName.ToString(), *GetNameSafe(FoundTable), *GetNameSafe(Table), *GetNameSafe(FoundTable));
            continue;
        }
        FoundRow = Row;
        FoundTable = Table;
    }

    if (OutTable != nullptr)
    {
        *OutTable = FoundTable;
    }
    return FoundRow;
}

#if WITH_EDITOR
void UKataDataCollection::WarnDuplicateRowName(const UDataTable* ChangedTable, FName RowName) const
{
    if (ChangedTable == nullptr || RowName.IsNone())
    {
        return;
    }

    const UDataTable* CharacterTables[] = { PlayerCharacterTable, NPCCharacterTable };
    if (!MakeArrayView(CharacterTables).Contains(ChangedTable))
    {
        return;
    }

    for (const UDataTable* OtherTable : CharacterTables)
    {
        if (OtherTable != nullptr && OtherTable != ChangedTable && OtherTable->FindRowUnchecked(RowName) != nullptr)
        {
            UE_LOG(LogKataFramework, Warning, TEXT("Kata Data: row %s in %s duplicates a row in %s. Row names must be unique within the character tables of %s."),
                *RowName.ToString(), *GetNameSafe(ChangedTable), *GetNameSafe(OtherTable), *GetNameSafe(this));
        }
    }
}

EDataValidationResult UKataDataCollection::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);

    TArray<const UDataTable*> Tables;
    GetCharacterTables(Tables);

    TArray<FText> Messages;
    CollectDuplicateRowNames(Tables, Messages);
    for (const FText& Message : Messages)
    {
        Context.AddWarning(Message);
    }
    return Result;
}

void UKataDataCollection::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    TArray<const UDataTable*> Tables;
    GetCharacterTables(Tables);

    TArray<FText> Messages;
    CollectDuplicateRowNames(Tables, Messages);
    for (const FText& Message : Messages)
    {
        UE_LOG(LogKataFramework, Warning, TEXT("Kata Data: %s"), *Message.ToString());
    }
}

void UKataDataCollection::CollectDuplicateRowNames(TConstArrayView<const UDataTable*> Tables, TArray<FText>& OutMessages)
{
    for (int32 Index = 0; Index < Tables.Num(); ++Index)
    {
        for (int32 OtherIndex = Index + 1; OtherIndex < Tables.Num(); ++OtherIndex)
        {
            for (const TPair<FName, uint8*>& Row : Tables[Index]->GetRowMap())
            {
                if (Tables[OtherIndex]->FindRowUnchecked(Row.Key) != nullptr)
                {
                    OutMessages.Add(FText::Format(
                        LOCTEXT("DuplicateRowName", "Row {0} exists in both {1} and {2}. Row names must be unique within the character tables."),
                        FText::FromName(Row.Key), FText::FromString(GetNameSafe(Tables[Index])), FText::FromString(GetNameSafe(Tables[OtherIndex]))));
                }
            }
        }
    }
}
#endif

#undef LOCTEXT_NAMESPACE
