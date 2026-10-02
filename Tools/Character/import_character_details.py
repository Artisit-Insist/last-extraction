import unreal,sys
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve();sys.path.insert(0,str(root/'Tools'))
from editor_shutdown import quit_after_notifications
assets=unreal.AssetToolsHelpers.get_asset_tools();ml=unreal.MaterialEditingLibrary
unreal.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX 0')
for name,folder,filename,masked in [('Commando_Boots03','clothes/shoes02','shoes02_diffuse.png',False),('Commando_Hair03','hair/short02','short02_diffuse.png',True)]:
 task=unreal.AssetImportTask();task.filename=str(root/'ArtSource/Production/MakeHumanSystemAssets'/folder/filename);task.destination_path='/Game/Production/Textures';task.automated=True;task.save=True;task.replace_existing=True;assets.import_asset_tasks([task])
 tex=unreal.load_asset('/Game/Production/Textures/'+Path(filename).stem)
 mat=unreal.load_asset('/Game/Production/Materials/M_'+name)
 if not mat:mat=assets.create_asset('M_'+name,'/Game/Production/Materials',unreal.Material,unreal.MaterialFactoryNew())
 ml.delete_all_material_expressions(mat);mat.set_editor_property('used_with_skeletal_mesh',True);mat.set_editor_property('two_sided',masked)
 sample=ml.create_material_expression(mat,unreal.MaterialExpressionTextureSample);sample.set_editor_property('texture',tex);ml.connect_material_property(sample,'RGB',unreal.MaterialProperty.MP_BASE_COLOR)
 rough=ml.create_material_expression(mat,unreal.MaterialExpressionConstant);rough.set_editor_property('r',.84);ml.connect_material_property(rough,'',unreal.MaterialProperty.MP_ROUGHNESS)
 if masked:mat.set_editor_property('blend_mode',unreal.BlendMode.BLEND_MASKED);mat.set_editor_property('opacity_mask_clip_value',.25);ml.connect_material_property(sample,'A',unreal.MaterialProperty.MP_OPACITY_MASK)
 ml.recompile_material(mat);unreal.EditorAssetLibrary.save_loaded_asset(mat,False)
opt=unreal.FbxImportUI();opt.import_mesh=True;opt.import_as_skeletal=True;opt.import_animations=False;opt.import_materials=False;opt.import_textures=False;opt.mesh_type_to_import=unreal.FBXImportType.FBXIT_SKELETAL_MESH;opt.automated_import_should_detect_type=False
opt.skeleton=unreal.load_asset('/Game/Production/Rigged/CommandoRigged_Skeleton')
task=unreal.AssetImportTask();task.filename=str(root/'ArtSource/Production/Export/CommandoVisual03.fbx');task.destination_path='/Game/Production/Rigged';task.destination_name='CommandoVisual03';task.automated=True;task.save=True;task.replace_existing=True;task.options=opt;assets.import_asset_tasks([task])
mesh=unreal.load_asset('/Game/Production/Rigged/CommandoVisual03');assert isinstance(mesh,unreal.SkeletalMesh)
slots=[]
for old in mesh.get_editor_property('materials'):
 mat=unreal.load_asset('/Game/Production/Materials/M_'+str(old.material_slot_name));assert mat,str(old.material_slot_name)
 slot=unreal.SkeletalMaterial();slot.material_slot_name=old.material_slot_name;slot.material_interface=mat;slots.append(slot)
mesh.set_editor_property('materials',slots);unreal.EditorAssetLibrary.save_loaded_asset(mesh,False)
unreal.log('CHARACTER_VISUAL03_IMPORTED skeleton='+mesh.get_editor_property('skeleton').get_path_name());quit_after_notifications()
