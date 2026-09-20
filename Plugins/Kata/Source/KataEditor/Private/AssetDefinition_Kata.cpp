#include "AssetDefinition_Kata.h"
#include "KataAssetEditor.h"

TConstArrayView<FAssetCategoryPath> UAssetDefinition_Kata::GetAssetCategories() const
{
    static const TArray<FAssetCategoryPath> Categories = { FAssetCategoryPath(NSLOCTEXT("Kata", "Category", "Kata")) };
    return Categories;
}

EAssetCommandResult UAssetDefinition_Kata::OpenAssets(const FAssetOpenArgs& OpenArgs) const
{
    for (UKataAsset* Asset : OpenArgs.LoadObjects<UKataAsset>())
    {
        MakeShared<FKataAssetEditor>()->Init(Asset);
    }
    return EAssetCommandResult::Handled;
}
