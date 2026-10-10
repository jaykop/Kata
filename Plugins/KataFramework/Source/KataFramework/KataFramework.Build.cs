using UnrealBuildTool;

public class KataFramework : ModuleRules
{
    public KataFramework(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // 공개 헤더가 ACharacter, IAbilitySystemInterface, IGenericTeamAgentInterface, UKataActionComponent, UKataGraphComponent,
        // UKataTargetingComponent, UKataTask, FGameplayTag, UDeveloperSettings, FInputActionValue를 노출하므로 Public에 둔다.
        // AnimGraphRuntime은 Kata Tilt 노드 헤더가 FAnimNode_SkeletalControlBase를 노출하므로 Public이다.
        PublicDependencyModuleNames.AddRange(new[]
        {
            "AIModule",
            "AnimGraphRuntime",
            "Core",
            "CoreUObject",
            "DeveloperSettings",
            "Engine",
            "EnhancedInput",
            "GameplayAbilities",
            "GameplayTags",
            "KataGraph",
            "KataRuntime",
            "KataTargeting",
            "KataAI",
            "UMG"
        });

        // Hit Trace의 대상 필터가 UTargetingPreset의 Filter 태스크를 실행한다. 공개 헤더는 전방 선언만 쓴다.
        // 플레이어 컨트롤러는 생성자에서만 AKataPlayerCameraManager를 참조한다.
        // Nav Mesh Projection 스포너 설정이 UNavigationSystemV1로 후보 위치를 NavMesh에 투영한다.
        // 피격 반응 Ability가 Ability Task(UGameplayTask 기반)로 반응 Kata와 몽타주를 재생한다.
        PrivateDependencyModuleNames.AddRange(new[]
        {
            "GameplayTasks",
            "KataCamera",
            "StateTreeModule",
            "NavigationSystem",
            "TargetingSystem",
            "SlateCore"
        });
    }
}
