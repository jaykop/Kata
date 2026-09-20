#include "AssetDefinition_KataGraph.h"

#include "KataGraphAssetEditor.h"

TConstArrayView<FAssetCategoryPath> UAssetDefinition_KataGraph::GetAssetCategories() const
{
    static const TArray<FAssetCategoryPath> Categories = { FAssetCategoryPath(NSLOCTEXT("Kata", "Category", "Kata")) };
    return Categories;
}

EAssetCommandResult UAssetDefinition_KataGraph::OpenAssets(const FAssetOpenArgs& OpenArgs) const
{
    for (UKataGraph* Graph : OpenArgs.LoadObjects<UKataGraph>())
    {
        TSharedRef<FKataGraphAssetEditor> Editor = MakeShared<FKataGraphAssetEditor>();
        Editor->InitKataGraphEditor(EToolkitMode::Standalone, TSharedPtr<IToolkitHost>(), Graph);
    }
    return EAssetCommandResult::Handled;
}
