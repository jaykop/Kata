using UnrealBuildTool;

public class KataFrameworkEditor : ModuleRules
{
    public KataFrameworkEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.Add("Core");

        // Kata 액션 에디터의 프리뷰 툴바를 확장한다. 메뉴 이름은 KataEditor의 공개 함수에서 받는다.
        PrivateDependencyModuleNames.AddRange(new[]
        {
            "CoreUObject",
            "Engine",
            "Slate",
            "SlateCore",
            "ToolMenus",
            "UnrealEd",
            "KataEditor",
            "KataFramework"
        });
    }
}
