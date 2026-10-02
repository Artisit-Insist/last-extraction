import unreal,sys,json
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve();sys.path.insert(0,str(root/'Tools'))
from editor_shutdown import quit_after_notifications
unreal.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX 0')
a=unreal.AssetToolsHelpers.get_asset_tools();opt=unreal.FbxImportUI();opt.import_mesh=True;opt.import_as_skeletal=True;opt.import_animations=True;opt.import_materials=False;opt.import_textures=False;opt.mesh_type_to_import=unreal.FBXImportType.FBXIT_SKELETAL_MESH;opt.automated_import_should_detect_type=False
opt.anim_sequence_import_data.set_editor_property('use_default_sample_rate',True)
t=unreal.AssetImportTask();t.filename=str(root/'ArtSource/Production/Export/CommandoRigged.fbx');t.destination_path='/Game/Production/Rigged';t.destination_name='CommandoRigged';t.automated=True;t.save=True;t.replace_existing=True;t.options=opt;a.import_asset_tasks([t])
for path in t.imported_object_paths:
 obj=unreal.load_asset(path)
 if isinstance(obj,unreal.SkeletalMesh):
  slots=[]
  for old in obj.get_editor_property('materials'):
   m=unreal.load_asset('/Game/Production/Materials/M_'+str(old.material_slot_name));assert m,str(old.material_slot_name);m.set_editor_property('used_with_skeletal_mesh',True);unreal.MaterialEditingLibrary.recompile_material(m);unreal.EditorAssetLibrary.save_loaded_asset(m,False)
   new=unreal.SkeletalMaterial();new.set_editor_property('material_slot_name',old.material_slot_name);new.set_editor_property('material_interface',m);slots.append(new)
  obj.set_editor_property('materials',slots);unreal.EditorAssetLibrary.save_loaded_asset(obj,False)
 unreal.log('RIGGED_ASSET '+path+' '+obj.get_class().get_name())
print('BLEND_FACTORIES',[n for n in dir(unreal) if 'BlendSpace' in n and 'Factory' in n])
print('BLEND_SAMPLE',unreal.BlendSample())
print('BLEND_PARAM',unreal.BlendParameter())
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True,True);assert unreal.CommandoAnimationTools.build_locomotion(),'BlendSpace build failed';unreal.log('RIGGED_IMPORT_READY');quit_after_notifications()
