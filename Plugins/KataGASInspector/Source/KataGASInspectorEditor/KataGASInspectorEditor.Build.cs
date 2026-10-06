using UnrealBuildTool;

public class KataGASInspectorEditor : ModuleRules
{
    public KataGASInspectorEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.AddRange(new[]
        {
            "Core", "CoreUObject", "Engine", "GameplayAbilities", "GameplayTags",
            "GameplayTasks", "Slate", "SlateCore", "UnrealEd", "ToolMenus",
            "ApplicationCore", "InputCore"
        });
    }
}
