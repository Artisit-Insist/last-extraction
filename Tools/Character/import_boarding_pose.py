import unreal,sys,json
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve();sys.path.insert(0,str(root/'Tools'))
from editor_shutdown import quit_after_notifications
unreal.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX 0')
a=unreal.AssetToolsHelpers.get_asset_tools();opt=unreal.FbxImportUI();opt.import_mesh=False;opt.import_as_skeletal=True;opt.import_animations=True;opt.import_materials=False;opt.import_textures=False;opt.mesh_type_to_import=unreal.FBXImportType.FBXIT_ANIMATION;opt.automated_import_should_detect_type=False
opt.anim_sequence_import_data.set_editor_property('use_default_sample_rate',True)
opt.skeleton=unreal.load_asset('/Game/Production/Rigged/CommandoRigged_Skeleton')
for name in ['CivilianSit']:
 t=unreal.AssetImportTask();t.filename=str(root/'ArtSource/Production/Export'/(name+'.fbx'));t.destination_path='/Game/Production/Rigged/Civilian';t.destination_name=name;t.automated=True;t.save=True;t.replace_existing=True;t.options=opt;a.import_asset_tasks([t])
 for path in t.imported_object_paths:
  obj=unreal.load_asset(path)
  if isinstance(obj,unreal.AnimSequence):
   target='/Game/Production/Rigged/Civilian/'+name
   if obj.get_path_name().split('.')[0]!=target:unreal.EditorAssetLibrary.rename_asset(path,target)
   unreal.log('CIVILIAN_ASSET '+target)
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True,True)
m=unreal.load_asset('/Game/Production/Materials/M_Transport_Glass')
m.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT)
m.set_editor_property('translucency_lighting_mode',unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
for value,prop in [(.38,unreal.MaterialProperty.MP_OPACITY),(0,unreal.MaterialProperty.MP_METALLIC),(.13,unreal.MaterialProperty.MP_ROUGHNESS)]:
 n=unreal.MaterialEditingLibrary.create_material_expression(m,unreal.MaterialExpressionConstant);n.set_editor_property('r',value);unreal.MaterialEditingLibrary.connect_material_property(n,'',prop)
unreal.MaterialEditingLibrary.recompile_material(m);unreal.EditorAssetLibrary.save_loaded_asset(m,False)
unreal.log('BOARDING_POSE_AND_GLASS_IMPORTED');quit_after_notifications()

