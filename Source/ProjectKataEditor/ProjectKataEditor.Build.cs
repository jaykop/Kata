using UnrealBuildTool;

public class ProjectKataEditor : ModuleRules
{
    public ProjectKataEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // 샘플 전용 에디터 도구 모듈이다. SlateIM(Experimental)은 샘플 .uproject에서 Editor 타깃에만 켠다.
        // 런타임 스포너 도구가 PIE 월드에 Kata 캐릭터를 생성하고 Action·Graph를 반복 실행한다.
        // GraphEditor는 검색·카테고리 선택 메뉴(SGraphActionMenu)에 쓴다.
        PrivateDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "UnrealEd",
            "Slate",
            "SlateCore",
            "SlateIM",
            "GraphEditor",
            "ToolMenus",
            "DeveloperSettings",
            "Settings",
            "AssetRegistry",
            "GameplayTags",
            "GameplayAbilities",
            "KataRuntime",
            "KataGraph",
            "KataAI",
            "KataFramework"
        });
    }
}
