using UnrealBuildTool;

public class KataFrameworkEditor : ModuleRules
{
    public KataFrameworkEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.Add("Core");

        // Kata 액션 에디터의 프리뷰 툴바를 확장한다. 메뉴 이름은 KataEditor의 공개 함수에서 받는다.
        // PropertyEditor는 데이터 행 ID의 행 이름 드롭다운 커스터마이즈에 쓴다.
        PrivateDependencyModuleNames.AddRange(new[]
        {
            "CoreUObject",
            "Engine",
            "PropertyEditor",
            "Slate",
            "SlateCore",
            "ToolMenus",
            "UnrealEd",
            "KataEditor",
            "KataFramework"
        });
    }
}
