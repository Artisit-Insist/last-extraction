import unreal,os
from pathlib import Path
root=str(Path(__file__).resolve().parent.parent)
a=unreal.AssetToolsHelpers.get_asset_tools()
unreal.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX 0')
for fname in os.listdir(root+'/ArtSource/Audio'):
 if not fname.endswith('.wav'):continue
 t=unreal.AssetImportTask();t.filename=root+'/ArtSource/Audio/'+fname;t.destination_path='/Game/Audio';t.automated=True;t.save=True;t.replace_existing=True;a.import_asset_tasks([t])
 obj=unreal.load_asset('/Game/Audio/'+fname[:-4])
 if obj and fname in ['Forest.wav','JungleScore.wav']:obj.set_editor_property('looping',True);unreal.EditorAssetLibrary.save_loaded_asset(obj)
for fname in os.listdir(root+'/ArtSource/Meshes'):
 if not fname.endswith('.fbx'):continue
 t=unreal.AssetImportTask();t.filename=root+'/ArtSource/Meshes/'+fname;t.destination_path='/Game/Meshes';t.automated=True;t.save=True;t.replace_existing=True
 opt=unreal.FbxImportUI();opt.set_editor_property('import_mesh',True);opt.set_editor_property('import_materials',False);opt.set_editor_property('import_textures',False);opt.set_editor_property('import_as_skeletal',False);opt.set_editor_property('mesh_type_to_import',unreal.FBXImportType.FBXIT_STATIC_MESH);opt.static_mesh_import_data.set_editor_property('combine_meshes',True);t.options=opt;a.import_asset_tasks([t])
# Verify assets instead of treating imports as proof.
for path in ['/Game/Art/JungleKeyArt','/Game/Art/EquipmentAtlas','/Game/Audio/JungleScore','/Game/Audio/Rifle','/Game/Meshes/Head','/Game/Meshes/Palm']:
 obj=unreal.load_asset(path);assert obj,path;unreal.log('VERIFIED_ASSET '+path)
unreal.log('PRODUCTION_ASSETS_READY')
