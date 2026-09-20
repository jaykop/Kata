#include "AssetDefinition_KataAction.h"
#include "KataActionEditor.h"

TConstArrayView<FAssetCategoryPath> UAssetDefinition_KataAction::GetAssetCategories() const
{
    static const TArray<FAssetCategoryPath> Categories = { FAssetCategoryPath(NSLOCTEXT("Kata", "Category", "Kata")) };
    return Categories;
}

EAssetCommandResult UAssetDefinition_KataAction::OpenAssets(const FAssetOpenArgs& OpenArgs) const
{
    for (UKataAction* Asset : OpenArgs.LoadObjects<UKataAction>())
    {
        MakeShared<FKataActionEditor>()->Init(Asset);
    }
    return EAssetCommandResult::Handled;
}
