using UnrealBuildTool;

public class KataFrameworkAnimGraph : ModuleRules
{
    public KataFrameworkAnimGraph(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // KataFramework 런타임 노드의 AnimGraph 편집기 노드를 둔다. 쿠킹하지 않은 실행에서도 ABP 그래프의 노드 클래스를 찾을 수 있도록
        // Editor가 아니라 UncookedOnly 모듈로 둔다.
        // 공개 헤더가 UAnimGraphNode_SkeletalControlBase(AnimGraph)와 FAnimNode_KataTilt(KataFramework, AnimGraphRuntime)를 노출한다.
        PublicDependencyModuleNames.AddRange(new[]
        {
            "AnimGraph",
            "AnimGraphRuntime",
            "Core",
            "CoreUObject",
            "Engine",
            "KataFramework"
        });

        // 편집기 노드의 그래프 스키마와 컴파일 처리는 에디터 빌드에서만 쓴다.
        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(new[]
            {
                "BlueprintGraph",
                "UnrealEd"
            });
        }
    }
}
