using UnrealBuildTool;

public class ProjectKataEditor : ModuleRules
{
    public ProjectKataEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // 샘플 전용 에디터 도구 모듈이다. SlateIM(Experimental)은 샘플 .uproject에서 Editor 타깃에만 켠다.
        // Kata Debug 탭이 PIE 월드를 대상으로 치트 패널과 런타임 스포너 패널을 그린다.
        // GraphEditor는 검색·카테고리 선택 메뉴(SGraphActionMenu)에, KataTargeting은 치트 대상 선택에 쓴다.
        // ToolsetRegistry와 ModelViewViewModel 계열은 HUD 위젯 저작을 돕는 MVVM 보조 툴셋에 쓴다.
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
            "KataTargeting",
            "KataFramework",
            "ToolsetRegistry",
            "UMG",
            "UMGEditor",
            "ModelViewViewModel",
            "ModelViewViewModelBlueprint",
            "ModelViewViewModelEditor"
        });
    }
}
