import unreal,sys,json
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve();sys.path.insert(0,str(root/'Tools'))
from editor_shutdown import quit_after_notifications
unreal.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX 0')
a=unreal.AssetToolsHelpers.get_asset_tools();opt=unreal.FbxImportUI();opt.import_mesh=False;opt.import_as_skeletal=True;opt.import_animations=True;opt.import_materials=False;opt.import_textures=False;opt.mesh_type_to_import=unreal.FBXImportType.FBXIT_ANIMATION;opt.automated_import_should_detect_type=False
opt.anim_sequence_import_data.set_editor_property('use_default_sample_rate',True)
opt.skeleton=unreal.load_asset('/Game/Production/Rigged/CommandoRigged_Skeleton')
for name in ['CivilianIdle','CivilianWalk','CivilianRun']:
 t=unreal.AssetImportTask();t.filename=str(root/'ArtSource/Production/Export'/(name+'.fbx'));t.destination_path='/Game/Production/Rigged/Civilian';t.destination_name=name;t.automated=True;t.save=True;t.replace_existing=True;t.options=opt;a.import_asset_tasks([t])
 for path in t.imported_object_paths:
  obj=unreal.load_asset(path)
  if isinstance(obj,unreal.AnimSequence):
   target='/Game/Production/Rigged/Civilian/'+name
   if obj.get_path_name().split('.')[0]!=target:unreal.EditorAssetLibrary.rename_asset(path,target)
   unreal.log('CIVILIAN_ASSET '+target)
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True,True)
assert unreal.CommandoAnimationTools.build_civilian_locomotion()
unreal.log('CIVILIAN_IMPORTED');quit_after_notifications()
