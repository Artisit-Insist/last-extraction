using UnrealBuildTool;
public class LastExtractionTarget : TargetRules {
 public LastExtractionTarget(TargetInfo Target) : base(Target) { Type=TargetType.Game; DefaultBuildSettings=BuildSettingsVersion.V7; IncludeOrderVersion=EngineIncludeOrderVersion.Unreal5_8; ExtraModuleNames.Add("LastExtraction"); }
}
