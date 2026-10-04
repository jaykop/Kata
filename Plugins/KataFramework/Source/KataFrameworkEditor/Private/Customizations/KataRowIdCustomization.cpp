#include "Customizations/KataRowIdCustomization.h"

#include "Data/KataRowId.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Engine/DataTable.h"
#include "PropertyHandle.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SSearchableComboBox.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "KataRowIdCustomization"

TSharedRef<IPropertyTypeCustomization> FKataRowIdCustomization::MakeInstance(FGetTables InGetTables)
{
    return MakeShared<FKataRowIdCustomization>(MoveTemp(InGetTables));
}

FKataRowIdCustomization::FKataRowIdCustomization(FGetTables InGetTables)
    : GetTables(MoveTemp(InGetTables))
{
}

void FKataRowIdCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow,
    IPropertyTypeCustomizationUtils& CustomizationUtils)
{
    RowNameHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FKataRowId, RowName));
    const FString& RowTypePath = PropertyHandle->GetMetaData(TEXT("RowType"));
    RowTypeFilter = !RowTypePath.IsEmpty() ? FindObject<UScriptStruct>(nullptr, *RowTypePath) : nullptr;
    SourceTablePropertyName = FName(*PropertyHandle->GetMetaData(TEXT("SourceTableProperty")));
    OuterObjects.Reset();
    TArray<UObject*> Outers;
    PropertyHandle->GetOuterObjects(Outers);
    for (UObject* Outer : Outers)
    {
        OuterObjects.Add(Outer);
    }
    RefreshOptions();

    HeaderRow
    .NameContent()
    [
        PropertyHandle->CreatePropertyNameWidget()
    ]
    .ValueContent()
    .MinDesiredWidth(200.f)
    [
        SAssignNew(ComboBox, SSearchableComboBox)
        .OptionsSource(&Options)
        .OnComboBoxOpening(this, &FKataRowIdCustomization::RefreshOptions)
        .OnSelectionChanged(this, &FKataRowIdCustomization::OnSelectionChanged)
        .OnGenerateWidget_Lambda([](TSharedPtr<FString> Item)
        {
            return SNew(STextBlock)
                .Text(FText::FromString(Item.IsValid() ? *Item : FString()))
                .Font(IDetailLayoutBuilder::GetDetailFont());
        })
        .ToolTipText(this, &FKataRowIdCustomization::GetToolTipText)
        .Content()
        [
            SNew(STextBlock)
            .Text(this, &FKataRowIdCustomization::GetCurrentText)
            .ColorAndOpacity(this, &FKataRowIdCustomization::GetCurrentColor)
            .Font(IDetailLayoutBuilder::GetDetailFont())
        ]
    ];
}

void FKataRowIdCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder,
    IPropertyTypeCustomizationUtils& CustomizationUtils)
{
    // RowName 하나뿐이라 헤더의 드롭다운으로 충분하다. 텍스트 칸을 따로 보여 주면 테이블에 없는 이름을 손으로 넣게 된다.
}

const UDataTable* FKataRowIdCustomization::GetSourceTable(bool& bOutMixed) const
{
    bOutMixed = false;
    if (SourceTablePropertyName.IsNone())
    {
        return nullptr;
    }

    const UDataTable* Result = nullptr;
    bool bFirst = true;
    for (const TWeakObjectPtr<UObject>& WeakOuter : OuterObjects)
    {
        const UObject* Outer = WeakOuter.Get();
        const FObjectPropertyBase* Property = Outer != nullptr
            ? FindFProperty<FObjectPropertyBase>(Outer->GetClass(), SourceTablePropertyName) : nullptr;
        const UDataTable* Table = Property != nullptr
            ? Cast<UDataTable>(Property->GetObjectPropertyValue_InContainer(Outer)) : nullptr;
        if (!bFirst && Table != Result)
        {
            // 여러 객체의 Source Table이 다르면 한 테이블로 좁힐 수 없으므로 거르지 않는다.
            bOutMixed = true;
            return nullptr;
        }
        Result = Table;
        bFirst = false;
    }
    return Result;
}

bool FKataRowIdCustomization::IsSourceTableUnregistered() const
{
    bool bMixed = false;
    const UDataTable* Source = GetSourceTable(bMixed);
    if (Source == nullptr)
    {
        return false;
    }
    TArray<const UDataTable*> Tables;
    GetTables(Tables);
    return !Tables.Contains(Source);
}

void FKataRowIdCustomization::GetFilteredTables(TArray<const UDataTable*>& OutTables) const
{
    GetTables(OutTables);
    const UScriptStruct* Filter = RowTypeFilter.Get();
    if (Filter != nullptr)
    {
        OutTables.RemoveAll([Filter](const UDataTable* Table)
        {
            return Table->GetRowStruct() == nullptr || !Table->GetRowStruct()->IsChildOf(Filter);
        });
    }

    bool bMixed = false;
    if (const UDataTable* Source = GetSourceTable(bMixed))
    {
        // 영역에 없는 테이블이면 목록을 비운다. ID는 영역 안에서만 찾히므로 그 테이블의 행을 고르게 하면 안 된다.
        OutTables.RemoveAll([Source](const UDataTable* Table) { return Table != Source; });
    }
}

void FKataRowIdCustomization::RefreshOptions()
{
    Options.Reset();
    // FName("None")은 NAME_None과 같으므로 이 항목을 고르면 값이 비워진다.
    Options.Add(MakeShared<FString>(FName(NAME_None).ToString()));

    TArray<const UDataTable*> Tables;
    GetFilteredTables(Tables);

    TArray<FName> RowNames;
    for (const UDataTable* Table : Tables)
    {
        for (const TPair<FName, uint8*>& Row : Table->GetRowMap())
        {
            RowNames.AddUnique(Row.Key);
        }
    }
    RowNames.Sort(FNameLexicalLess());
    for (const FName RowName : RowNames)
    {
        Options.Add(MakeShared<FString>(RowName.ToString()));
    }

    if (ComboBox.IsValid())
    {
        ComboBox->RefreshOptions();
    }
}

void FKataRowIdCustomization::OnSelectionChanged(TSharedPtr<FString> SelectedItem, ESelectInfo::Type SelectInfo)
{
    // 코드에서 선택을 바꾼 경우(Direct)는 값을 쓰지 않는다. 목록을 다시 채울 때도 이 콜백이 불린다.
    if (!SelectedItem.IsValid() || SelectInfo == ESelectInfo::Direct || !RowNameHandle.IsValid())
    {
        return;
    }
    RowNameHandle->SetValue(FName(**SelectedItem));
}

bool FKataRowIdCustomization::GetCurrentRowName(FName& OutRowName) const
{
    return RowNameHandle.IsValid() && RowNameHandle->GetValue(OutRowName) == FPropertyAccess::Success;
}

bool FKataRowIdCustomization::IsCurrentRowMissing() const
{
    FName RowName;
    if (!GetCurrentRowName(RowName) || RowName.IsNone())
    {
        return false;
    }

    TArray<const UDataTable*> Tables;
    GetFilteredTables(Tables);
    for (const UDataTable* Table : Tables)
    {
        if (Table->FindRowUnchecked(RowName) != nullptr)
        {
            return false;
        }
    }
    return true;
}

FText FKataRowIdCustomization::GetCurrentText() const
{
    FName RowName;
    if (!GetCurrentRowName(RowName))
    {
        return LOCTEXT("MultipleValues", "Multiple Values");
    }
    if (IsCurrentRowMissing())
    {
        return FText::Format(LOCTEXT("MissingRow", "{0} (missing)"), FText::FromName(RowName));
    }
    return FText::FromName(RowName);
}

FSlateColor FKataRowIdCustomization::GetCurrentColor() const
{
    return IsCurrentRowMissing() ? FSlateColor(FAppStyle::Get().GetSlateColor("Colors.Error")) : FSlateColor::UseForeground();
}

FText FKataRowIdCustomization::GetToolTipText() const
{
    TArray<const UDataTable*> Tables;
    GetFilteredTables(Tables);
    if (IsSourceTableUnregistered())
    {
        return LOCTEXT("SourceTableUnregistered", "The Source Table is not registered in the current Data Collection. Add it to the collection's NPC Character Tables or pick another table.");
    }
    if (Tables.IsEmpty())
    {
        // 컬렉션 자체가 없거나 비었는지, 컬렉션은 있지만 이 프로퍼티의 행 구조에 맞는 테이블이 없는지를 나눠 알린다.
        TArray<const UDataTable*> AllTables;
        GetTables(AllTables);
        if (AllTables.IsEmpty())
        {
            return LOCTEXT("NoTables", "No tables to choose from. Set a Data Collection in Project Settings > Game > Kata Data and assign its tables.");
        }
        return FText::Format(LOCTEXT("NoMatchingTables", "The current Data Collection has no table with row type {0}. Assign one in the collection."),
            FText::FromString(GetNameSafe(RowTypeFilter.Get())));
    }
    if (IsCurrentRowMissing())
    {
        return LOCTEXT("MissingRowTooltip", "This row is not in the tables of the current Data Collection. It may have been renamed or removed.");
    }
    return LOCTEXT("PickRowTooltip", "Pick a row from the tables of the current Data Collection.");
}

#undef LOCTEXT_NAMESPACE
