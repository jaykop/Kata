using UnrealBuildTool;

public class KataCamera : ModuleRules
{
    public KataCamera(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // 공개 헤더가 APlayerCameraManager, UPrimaryDataAsset을 노출하므로 Public에 둔다.
        // 코어 Kata와 GAS는 이를 실제로 쓰는 단계(CAM-4 ASC, 카메라 태스크)에서 추가한다.
        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine"
        });

        // 디버그 카테고리는 GameplayDebugger를 쓸 수 있는 대상에서만 빌드된다. Shipping 등에서는 WITH_GAMEPLAY_DEBUGGER가 0이다.
        SetupGameplayDebuggerSupport(Target);
    }
}
