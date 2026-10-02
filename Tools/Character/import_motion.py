import unreal,sys,json
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve();sys.path.insert(0,str(root/'Tools'))
from editor_shutdown import quit_after_notifications
unreal.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX 0')
a=unreal.AssetToolsHelpers.get_asset_tools();opt=unreal.FbxImportUI();opt.import_mesh=False;opt.import_as_skeletal=True;opt.import_animations=True;opt.import_materials=False;opt.import_textures=False;opt.mesh_type_to_import=unreal.FBXImportType.FBXIT_ANIMATION;opt.automated_import_should_detect_type=False
opt.anim_sequence_import_data.set_editor_property('use_default_sample_rate',True)
opt.skeleton=unreal.load_asset('/Game/Production/Rigged/CommandoRigged_Skeleton');t=unreal.AssetImportTask();t.filename=str(root/'ArtSource/Production/Export/CommandoRigged.fbx');t.destination_path='/Game/Production/Rigged/Motions';t.destination_name='Motion';t.automated=True;t.save=True;t.replace_existing=True;t.options=opt;a.import_asset_tasks([t])

for path in unreal.EditorAssetLibrary.list_assets('/Game/Production/Rigged/Motions'):
 obj=unreal.load_asset(path)
 if isinstance(obj,unreal.AnimSequence):unreal.log('ANIM_READY '+obj.get_name())
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True,True)
assert unreal.CommandoAnimationTools.build_locomotion(),'BlendSpace build failed'
unreal.log('RIGGED_IMPORT_READY');quit_after_notifications()
