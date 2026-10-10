using UnrealBuildTool;

public class KataFrameworkEditor : ModuleRules
{
    public KataFrameworkEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.Add("Core");

        // Kata 액션 에디터의 프리뷰 툴바를 확장한다. 메뉴 이름은 KataEditor의 공개 함수에서 받는다.
        // PropertyEditor는 데이터 행 ID의 행 이름 드롭다운 커스터마이즈에 쓴다.
        // AnimationModifiers는 루트 모션 커브 추출 수정자의 기반 클래스이며, 그 헤더가 AnimationBlueprintLibrary 헤더를 포함한다.
        // ContentBrowser는 몽타주 우클릭 메뉴의 선택 에셋 컨텍스트에 쓴다.
        PrivateDependencyModuleNames.AddRange(new[]
        {
            "AnimationBlueprintLibrary",
            "AnimationModifiers",
            "ContentBrowser",
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
