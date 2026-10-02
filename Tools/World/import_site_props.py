"""Import Props08 only; do not rebuild earlier water, surfaces, or models."""
from pathlib import Path
import unreal,json,sys
root=Path(unreal.Paths.project_dir()).resolve();sys.path.insert(0,str(root/'Tools'));from editor_shutdown import quit_after_notifications
source=root/'ArtSource/Production/Props08';manifest=json.loads((source/'site_props_manifest.json').read_text());at=unreal.AssetToolsHelpers.get_asset_tools();ml=unreal.MaterialEditingLibrary
unreal.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX 0')
def material(info):
 n=info['name']
 for folder,prefix in [('Materials04','M04_'),('Materials','M_')]:
  m=unreal.load_asset('/Game/Production/'+folder+'/'+prefix+n)
  if m:return m
 path='/Game/Production/Materials08/M08_'+n;m=unreal.load_asset(path)
 if not m:
  m=at.create_asset('M08_'+n,'/Game/Production/Materials08',unreal.Material,unreal.MaterialFactoryNew());m.set_editor_property('two_sided',True)
  col=ml.create_material_expression(m,unreal.MaterialExpressionConstant3Vector);col.set_editor_property('constant',unreal.LinearColor(*info['base']));ml.connect_material_property(col,'',unreal.MaterialProperty.MP_BASE_COLOR)
  for v,p in [(info['roughness'],unreal.MaterialProperty.MP_ROUGHNESS),(info['metallic'],unreal.MaterialProperty.MP_METALLIC)]:
   c=ml.create_material_expression(m,unreal.MaterialExpressionConstant);c.set_editor_property('r',v);ml.connect_material_property(c,'',p)
  ml.recompile_material(m);unreal.EditorAssetLibrary.save_loaded_asset(m,False)
 return m
for name,entry in manifest.items():
 materials={i['name']:material(i) for i in entry['materials']};opt=unreal.FbxImportUI();opt.import_mesh=True;opt.import_materials=False;opt.import_textures=False;opt.import_as_skeletal=False;opt.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH;opt.static_mesh_import_data.combine_meshes=True
 t=unreal.AssetImportTask();t.filename=str(source/(name+'.fbx'));t.destination_path='/Game/Production/Meshes';t.destination_name=name;t.automated=True;t.replace_existing=True;t.save=True;t.options=opt;at.import_asset_tasks([t]);mesh=unreal.load_asset('/Game/Production/Meshes/'+name);assert mesh,name
 for i,slot in enumerate(mesh.get_editor_property('static_materials')):
  key=str(slot.material_slot_name);assert key in materials,(name,key,list(materials));mesh.set_material(i,materials[key])
 mesh.get_editor_property('body_setup').set_editor_property('collision_trace_flag',unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
 unreal.EditorAssetLibrary.save_loaded_asset(mesh,False);unreal.log('SITE_PROP08_IMPORTED '+name)
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True,True);unreal.log('SITE_PROPS08_IMPORT_COMPLETE');quit_after_notifications()
