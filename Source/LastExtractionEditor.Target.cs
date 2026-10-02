using UnrealBuildTool;
public class LastExtractionEditorTarget : TargetRules {
 public LastExtractionEditorTarget(TargetInfo Target) : base(Target) { Type=TargetType.Editor; DefaultBuildSettings=BuildSettingsVersion.V7; IncludeOrderVersion=EngineIncludeOrderVersion.Unreal5_8; ExtraModuleNames.Add("LastExtraction"); }
}
