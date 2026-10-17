using UnrealBuildTool;

public class KataWorld : ModuleRules
{
    public KataWorld(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // 공개 헤더가 엔진 타입, FGameplayTag, UDeveloperSettings를 노출하므로 Public에 둔다.
        // 코어 Kata는 조건·레벨 이벤트처럼 실제로 쓰는 단계에서 추가한다.
        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "GameplayTags",
            "DeveloperSettings"
        });
    }
}
