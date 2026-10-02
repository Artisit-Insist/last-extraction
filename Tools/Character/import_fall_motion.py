"""Import authored falls and create a separate collision-enabled rifle for drops."""
import unreal,sys,json
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve();sys.path.insert(0,str(root/'Tools'))
from editor_shutdown import quit_after_notifications
at=unreal.AssetToolsHelpers.get_asset_tools();unreal.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX 0')
opt=unreal.FbxImportUI();opt.import_mesh=False;opt.import_as_skeletal=True;opt.import_animations=True;opt.import_materials=False;opt.import_textures=False;opt.mesh_type_to_import=unreal.FBXImportType.FBXIT_ANIMATION;opt.automated_import_should_detect_type=False
opt.anim_sequence_import_data.set_editor_property('use_default_sample_rate',True);opt.skeleton=unreal.load_asset('/Game/Production/Rigged/CommandoRigged_Skeleton')
for name in json.loads((root/'ArtSource/Production/Animation07/fall_manifest.json').read_text()):
 t=unreal.AssetImportTask();t.filename=str(root/'ArtSource/Production/Animation07'/(name+'.fbx'));t.destination_path='/Game/Production/Rigged/Animation07';t.destination_name=name;t.automated=True;t.save=True;t.replace_existing=True;t.options=opt;at.import_asset_tasks([t])
 for path in t.imported_object_paths:
  obj=unreal.load_asset(path)
  if isinstance(obj,unreal.AnimSequence):
   target='/Game/Production/Rigged/Animation07/'+name
   if obj.get_path_name().split('.')[0]!=target:assert unreal.EditorAssetLibrary.rename_asset(path,target)
   unreal.log('FALL07_IMPORTED '+target)
path='/Game/Production/Meshes/FieldRifleDrop07';mesh=unreal.load_asset(path)
if not mesh:mesh=unreal.EditorAssetLibrary.duplicate_asset('/Game/Production/Meshes/FieldRifle',path)
assert mesh
sub=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
assert sub.set_convex_decomposition_collisions(mesh,5,24,100000),'Weapon convex collision build failed'
body=mesh.get_editor_property('body_setup');body.set_editor_property('collision_trace_flag',unreal.CollisionTraceFlag.CTF_USE_SIMPLE_AND_COMPLEX)
physics=unreal.load_asset('/Game/Production/Materials07/PM_DroppedSteel')
if not physics:physics=at.create_asset('PM_DroppedSteel','/Game/Production/Materials07',unreal.PhysicalMaterial,unreal.PhysicalMaterialFactoryNew())
physics.set_editor_property('friction',.7);physics.set_editor_property('restitution',.12);body.set_editor_property('phys_material',physics)
unreal.EditorAssetLibrary.save_loaded_asset(physics,False);unreal.EditorAssetLibrary.save_loaded_asset(mesh,False)
unreal.log('DROPPED_RIFLE_PHYSICS_READY hulls='+str(sub.get_convex_collision_count(mesh)))
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True,True);unreal.log('FALL07_IMPORT_COMPLETE');quit_after_notifications()
