using UnrealBuildTool;

public class KataGraphEditor : ModuleRules
{
    public KataGraphEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "UnrealEd"
        });

        PrivateDependencyModuleNames.AddRange(new[]
        {
            "KataGraph",
            "KataRuntime",
            "ApplicationCore",
            "AssetDefinition",
            "AssetTools",
            "GraphEditor",
            "InputCore",
            "Kismet",
            "KismetWidgets",
            "PropertyEditor",
            "Slate",
            "SlateCore",
            "ToolMenus"
        });
    }
}
