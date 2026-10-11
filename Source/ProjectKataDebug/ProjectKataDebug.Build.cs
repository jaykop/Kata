using UnrealBuildTool;

public class ProjectKataDebug : ModuleRules
{
    public ProjectKataDebug(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // 샘플 전용 인게임 디버그 메뉴 모듈이다. DeveloperTool 타입이라 Editor와 Game의 Debug·Development에서만 빌드된다.
        // SlateIM(Experimental)은 샘플 .uproject에서 Shipping·Test 구성만 빼고 켠다. Kata 플러그인은 SlateIM을 참조하지 않는다.
        PrivateDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "Slate",
            "SlateCore",
            "SlateIM",
            "GameplayAbilities",
            "GameplayTags",
            "AIModule",
            "KataTargeting",
            "KataAI",
            "KataFramework"
        });
    }
}
