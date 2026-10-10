#include "AssetDefinition_KataActionTemplate.h"
#include "KataActionEditor.h"

TConstArrayView<FAssetCategoryPath> UAssetDefinition_KataActionTemplate::GetAssetCategories() const
{
    static const TArray<FAssetCategoryPath> Categories = { FAssetCategoryPath(NSLOCTEXT("Kata", "Category", "Kata")) };
    return Categories;
}

EAssetCommandResult UAssetDefinition_KataActionTemplate::OpenAssets(const FAssetOpenArgs& OpenArgs) const
{
    for (UKataActionTemplate* Asset : OpenArgs.LoadObjects<UKataActionTemplate>())
    {
        MakeShared<FKataActionEditor>()->Init(Asset);
    }
    return EAssetCommandResult::Handled;
}
