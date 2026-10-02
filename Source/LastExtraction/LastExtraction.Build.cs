using UnrealBuildTool;
public class LastExtraction : ModuleRules {
 public LastExtraction(ReadOnlyTargetRules Target) : base(Target) {
  PCHUsage=PCHUsageMode.UseExplicitOrSharedPCHs;
  PublicDependencyModuleNames.AddRange(new string[]{"Core","CoreUObject","Engine","InputCore","Slate","SlateCore","Json","JsonUtilities","RenderCore","AnimGraphRuntime","AssetRegistry","ProceduralMeshComponent"});
 }
}
