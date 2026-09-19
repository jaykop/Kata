using UnrealBuildTool;

public class KataEditor : ModuleRules
{
    public KataEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.Add("Core");

        PrivateDependencyModuleNames.AddRange(new[]
        {
            "CoreUObject",
            "Engine",
            "KataConditions",
            "KataRuntime",
            "UnrealEd",
            "Slate",
            "SlateCore"
        });
    }
}
