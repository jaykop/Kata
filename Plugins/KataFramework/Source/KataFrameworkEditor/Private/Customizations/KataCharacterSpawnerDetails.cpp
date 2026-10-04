#include "Customizations/KataCharacterSpawnerDetails.h"

#include "AssetRegistry/AssetData.h"
#include "Data/KataDataCollection.h"
#include "Data/KataDataSettings.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Engine/DataTable.h"
#include "IDetailPropertyRow.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyHandle.h"
#include "Spawning/KataCharacterSpawner.h"

TSharedRef<IDetailCustomization> FKataCharacterSpawnerDetails::MakeInstance()
{
    return MakeShared<FKataCharacterSpawnerDetails>();
}

void FKataCharacterSpawnerDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
    const TSharedRef<IPropertyHandle> SourceTableHandle =
        DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(AKataCharacterSpawner, SourceTable));
    if (!SourceTableHandle->IsValidHandle())
    {
        return;
    }

    // 기본 행 자리를 유지한 채 값 위젯만 바꾼다. 카테고리 순서와 리셋·복사 메뉴는 기본 행이 그대로 제공한다.
    IDetailPropertyRow* Row = DetailBuilder.EditDefaultProperty(SourceTableHandle);
    if (Row == nullptr)
    {
        return;
    }
    Row->CustomWidget()
    .NameContent()
    [
        SourceTableHandle->CreatePropertyNameWidget()
    ]
    .ValueContent()
    .MinDesiredWidth(250.f)
    [
        SNew(SObjectPropertyEntryBox)
        .PropertyHandle(SourceTableHandle)
        .AllowedClass(UDataTable::StaticClass())
        .OnShouldFilterAsset_Static(&FKataCharacterSpawnerDetails::ShouldFilterSourceTable)
        .ThumbnailPool(DetailBuilder.GetThumbnailPool())
    ];
}

bool FKataCharacterSpawnerDetails::ShouldFilterSourceTable(const FAssetData& AssetData)
{
    // 실행 중 검사(AKataCharacterSpawner::SpawnCharacters)와 같은 기준을 쓴다.
    const UKataDataCollection* Collection = UKataDataSettings::Get()->GetDataCollection();
    if (Collection == nullptr)
    {
        return true;
    }

    const FSoftObjectPath AssetPath = AssetData.GetSoftObjectPath();
    for (const TObjectPtr<UDataTable>& Table : Collection->NPCCharacterTables)
    {
        if (Table != nullptr && FSoftObjectPath(Table.Get()) == AssetPath)
        {
            return !Collection->IsNPCCharacterTable(Table);
        }
    }
    return true;
}
