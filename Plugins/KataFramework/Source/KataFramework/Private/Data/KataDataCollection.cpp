#include "Data/KataDataCollection.h"

#include "Character/KataCharacterRow.h"
#include "Equipment/KataEquipmentRow.h"
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
    for (const TObjectPtr<UDataTable>& NPCTable : NPCCharacterTables)
    {
        if (const UDataTable* Table = GetCheckedTable(NPCTable, FKataNPCCharacterRow::StaticStruct()))
        {
            OutTables.AddUnique(Table);
        }
    }
}

bool UKataDataCollection::IsNPCCharacterTable(const UDataTable* Table) const
{
    return Table != nullptr && NPCCharacterTables.Contains(Table) && GetCheckedTable(Table, FKataNPCCharacterRow::StaticStruct()) != nullptr;
}

const FKataEquipmentRow* UKataDataCollection::FindEquipmentRow(FName RowName, const UDataTable** OutTable) const
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
    GetEquipmentTables(Tables);

    // GetCheckedTable이 FKataEquipmentRow 계열 행 구조만 통과시키므로 기반 타입으로 해석해도 된다.
    return reinterpret_cast<const FKataEquipmentRow*>(FindRowInTables(Tables, RowName, OutTable));
}

void UKataDataCollection::GetEquipmentTables(TArray<const UDataTable*>& OutTables) const
{
    OutTables.Reset();
    for (const TObjectPtr<UDataTable>& EquipmentTable : EquipmentTables)
    {
        if (const UDataTable* Table = GetCheckedTable(EquipmentTable, FKataEquipmentRow::StaticStruct()))
        {
            OutTables.AddUnique(Table);
        }
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

    // 바뀐 테이블이 속한 영역 안에서만 비교한다. 캐릭터와 장비는 영역이 달라 같은 이름을 써도 된다.
    TArray<const UDataTable*> AreaTables;
    GetCharacterTables(AreaTables);
    if (!AreaTables.Contains(ChangedTable))
    {
        GetEquipmentTables(AreaTables);
        if (!AreaTables.Contains(ChangedTable))
        {
            return;
        }
    }

    for (const UDataTable* OtherTable : AreaTables)
    {
        if (OtherTable != nullptr && OtherTable != ChangedTable && OtherTable->FindRowUnchecked(RowName) != nullptr)
        {
            UE_LOG(LogKataFramework, Warning, TEXT("Kata Data: row %s in %s duplicates a row in %s. Row names must be unique within the same data area of %s."),
                *RowName.ToString(), *GetNameSafe(ChangedTable), *GetNameSafe(OtherTable), *GetNameSafe(this));
        }
    }
}

EDataValidationResult UKataDataCollection::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);

    TArray<FText> Messages;
    CollectAllDuplicateRowNames(Messages);
    for (const FText& Message : Messages)
    {
        Context.AddWarning(Message);
    }
    return Result;
}

void UKataDataCollection::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    TArray<FText> Messages;
    CollectAllDuplicateRowNames(Messages);
    for (const FText& Message : Messages)
    {
        UE_LOG(LogKataFramework, Warning, TEXT("Kata Data: %s"), *Message.ToString());
    }
}

void UKataDataCollection::CollectAllDuplicateRowNames(TArray<FText>& OutMessages) const
{
    TArray<const UDataTable*> Tables;
    GetCharacterTables(Tables);
    CollectDuplicateRowNames(Tables, OutMessages);
    GetEquipmentTables(Tables);
    CollectDuplicateRowNames(Tables, OutMessages);
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
                        LOCTEXT("DuplicateRowName", "Row {0} exists in both {1} and {2}. Row names must be unique within the same data area."),
                        FText::FromName(Row.Key), FText::FromString(GetNameSafe(Tables[Index])), FText::FromString(GetNameSafe(Tables[OtherIndex]))));
                }
            }
        }
    }
}
#endif

#undef LOCTEXT_NAMESPACE
