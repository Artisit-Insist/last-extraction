"""Import new wardrobe assets without changing the original character or skeleton."""
import unreal,json,sys
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve();source=root/'ArtSource/Production';sys.path.insert(0,str(root/'Tools'))
from editor_shutdown import quit_after_notifications
at=unreal.AssetToolsHelpers.get_asset_tools();ml=unreal.MaterialEditingLibrary
unreal.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX 0')
manifest=json.loads((source/'Characters06/materials.json').read_text());textures={};materials={}
def node(m,cls,**props):
 n=ml.create_material_expression(m,cls)
 for k,v in props.items():n.set_editor_property(k,v)
 return n
def con(a,ap,b,bp):assert ml.connect_material_expressions(a,ap,b,bp)
def prop(m,n,p,pin=''):assert ml.connect_material_property(n,pin,p)
def texture(path,normal=False,linear=False):
 key=(path,normal,linear)
 if key in textures:return textures[key]
 f=source/path;name=f.stem+('_linear' if linear else '')
 dest='/Game/Production/Textures06/'+name;t=unreal.load_asset(dest)
 if not t:
  task=unreal.AssetImportTask();task.filename=str(f);task.destination_path='/Game/Production/Textures06';task.destination_name=name;task.automated=True;task.save=True;task.replace_existing=False;at.import_asset_tasks([task]);t=unreal.load_asset(dest)
 assert t,dest
 t.set_editor_property('srgb',not(normal or linear))
 # Blue denim can trigger Unreal's automatic normal-map detection. Explicitly
 # classify albedo so ordinary blue cloth is not decoded as a surface normal.
 t.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_NORMALMAP if normal else unreal.TextureCompressionSettings.TC_DEFAULT)
 t.set_editor_property('flip_green_channel',normal)
 unreal.EditorAssetLibrary.save_loaded_asset(t,False);textures[key]=t;return t
for name,info in manifest.items():
 if name=='Role06_Boots':materials[name]=unreal.load_asset('/Game/Production/Materials04/M04_CombatBoots');continue
 m=unreal.load_asset('/Game/Production/Materials06/M_'+name)
 if not m:m=at.create_asset('M_'+name,'/Game/Production/Materials06',unreal.Material,unreal.MaterialFactoryNew())
 for old in list(ml.get_material_expressions(m)):ml.delete_material_expression(m,old)
 m.set_editor_property('used_with_skeletal_mesh',True);m.set_editor_property('two_sided',info['masked'])
 color=node(m,unreal.MaterialExpressionVectorParameter,parameter_name='Tint',default_value=unreal.LinearColor(*info['color'],1))
 if info['texture']:
  color=node(m,unreal.MaterialExpressionTextureSample,texture=texture(info['texture']))
  if info['texture']=='Camouflage.png':
   uv=node(m,unreal.MaterialExpressionTextureCoordinate,u_tiling=3.0,v_tiling=3.0);con(uv,'',color,'UVs')
 elif info['ao']:
  ao=node(m,unreal.MaterialExpressionTextureSample,texture=texture(info['ao'],linear=True),sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
  mul=node(m,unreal.MaterialExpressionMultiply);con(color,'',mul,'A');con(ao,'RGB',mul,'B');color=mul
 prop(m,color,unreal.MaterialProperty.MP_BASE_COLOR,'RGB' if info['texture'] else '')
 prop(m,node(m,unreal.MaterialExpressionConstant,r=info['roughness']),unreal.MaterialProperty.MP_ROUGHNESS)
 if info['masked']:
  m.set_editor_property('blend_mode',unreal.BlendMode.BLEND_MASKED);m.set_editor_property('opacity_mask_clip_value',.25);prop(m,color,unreal.MaterialProperty.MP_OPACITY_MASK,'A')
 if info['normal']:
  norm=node(m,unreal.MaterialExpressionTextureSample,texture=texture(info['normal'],normal=True),sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL);prop(m,norm,unreal.MaterialProperty.MP_NORMAL,'RGB')
 ml.recompile_material(m);unreal.EditorAssetLibrary.save_loaded_asset(m,False);materials[name]=m
opt=unreal.FbxImportUI();opt.import_mesh=True;opt.import_as_skeletal=True;opt.import_animations=False;opt.import_materials=False;opt.import_textures=False;opt.mesh_type_to_import=unreal.FBXImportType.FBXIT_SKELETAL_MESH;opt.automated_import_should_detect_type=False
opt.skeleton=unreal.load_asset('/Game/Production/Rigged/CommandoRigged_Skeleton')
for name in json.loads((source/'Characters06/manifest.json').read_text()):
 task=unreal.AssetImportTask();task.filename=str(source/'Characters06'/(name+'.fbx'));task.destination_path='/Game/Production/Rigged/Characters06';task.destination_name=name;task.automated=True;task.save=True;task.replace_existing=True;task.options=opt;at.import_asset_tasks([task])
 mesh=unreal.load_asset('/Game/Production/Rigged/Characters06/'+name);assert isinstance(mesh,unreal.SkeletalMesh),name
 assert mesh.get_editor_property('skeleton')==opt.skeleton,name+' incompatible skeleton'
 slots=[]
 for old in mesh.get_editor_property('materials'):
  mat=materials[str(old.material_slot_name)];assert mat,str(old.material_slot_name)
  slot=unreal.SkeletalMaterial();slot.material_slot_name=old.material_slot_name;slot.material_interface=mat;slots.append(slot)
 mesh.set_editor_property('materials',slots);unreal.EditorAssetLibrary.save_loaded_asset(mesh,False);unreal.log('ROLE06_IMPORTED '+name)
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True,True)
unreal.log('CHARACTER_ROLES06_IMPORT_COMPLETE');quit_after_notifications()
