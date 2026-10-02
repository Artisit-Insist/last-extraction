import sys as _shutdown_sys
import pathlib as _shutdown_paths
import unreal as _shutdown_unreal
_shutdown_sys.path.insert(0, str(_shutdown_paths.Path(_shutdown_unreal.Paths.project_dir()).resolve() / 'Tools'))
from editor_shutdown import quit_after_notifications as _quit_after_notifications
import unreal,json,re
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve()/'ArtSource/Production';out=root/'Export';a=unreal.AssetToolsHelpers.get_asset_tools();ml=unreal.MaterialEditingLibrary
unreal.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX 0')
def imp(path,dest,name=None,opts=None):
 t=unreal.AssetImportTask();t.filename=str(path);t.destination_path=dest;t.automated=True;t.save=True;t.replace_existing=True
 if name:t.destination_name=name
 if opts:t.options=opts
 a.import_asset_tasks([t]);return [unreal.load_asset(p) for p in t.imported_object_paths]
def node(m,cls,**props):
 n=ml.create_material_expression(m,cls)
 for k,v in props.items():n.set_editor_property(k,v)
 return n
def wire(n,p,m,prop):assert ml.connect_material_property(n,p,prop),(n,p,prop)
def tex(path,normal=False,linear=False):
 n=Path(path).stem;n=re.sub('[^a-zA-Z0-9_]','_',n);p='/Game/Production/Textures/'+n;t=unreal.load_asset(p)
 if not t:
  imp(path,'/Game/Production/Textures',n);t=unreal.load_asset(p)
 if normal:t.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_NORMALMAP);t.set_editor_property('flip_green_channel',True)
 if normal or linear:t.set_editor_property('srgb',False)
 t.set_editor_property('never_stream',True);unreal.EditorAssetLibrary.save_loaded_asset(t);return t
def material(info):
 n='M_'+info['name'];p='/Game/Production/Materials/'+n;m=unreal.load_asset(p)
 if m:return m
 m=a.create_asset(n,'/Game/Production/Materials',unreal.Material,unreal.MaterialFactoryNew());m.set_editor_property('two_sided',True)
 textures=info.get('textures',{});hasdiff=False
 for name,path in textures.items():
  marker='ArtSource/Production/'
  if marker in str(path):path=root/str(path).split(marker,1)[1]
  normal='_nor_' in name;linear=normal or '_arm_' in name or '_alpha_' in name
  t=tex(path,normal,linear);sam=node(m,unreal.MaterialExpressionTextureSample,texture=t)
  if normal:sam.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL);wire(sam,'RGB',m,unreal.MaterialProperty.MP_NORMAL)
  elif '_diff_' in name:wire(sam,'RGB',m,unreal.MaterialProperty.MP_BASE_COLOR);hasdiff=True
  elif '_arm_' in name:
   sam.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR);wire(sam,'R',m,unreal.MaterialProperty.MP_AMBIENT_OCCLUSION);wire(sam,'G',m,unreal.MaterialProperty.MP_ROUGHNESS)
  elif '_alpha_' in name:
   sam.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR);m.set_editor_property('blend_mode',unreal.BlendMode.BLEND_MASKED);wire(sam,'R',m,unreal.MaterialProperty.MP_OPACITY_MASK)
 if not hasdiff:
  c=info.get('base',[.1,.15,.08,1]);v=node(m,unreal.MaterialExpressionConstant3Vector,constant=unreal.LinearColor(*c));wire(v,'',m,unreal.MaterialProperty.MP_BASE_COLOR)
  v=node(m,unreal.MaterialExpressionConstant,r=float(info.get('roughness',.7)));wire(v,'',m,unreal.MaterialProperty.MP_ROUGHNESS)
  v=node(m,unreal.MaterialExpressionConstant,r=float(info.get('metallic',0)));wire(v,'',m,unreal.MaterialProperty.MP_METALLIC)
 if info['name']=='Commando_Camo':
  t=tex(root/'Camouflage.png');sam=node(m,unreal.MaterialExpressionTextureSample,texture=t);wire(sam,'RGB',m,unreal.MaterialProperty.MP_BASE_COLOR)
 ml.recompile_material(m);unreal.EditorAssetLibrary.save_loaded_asset(m);return m
manifest=json.loads((out/'landmark_manifest.json').read_text())
for name,entry in manifest.items():
 mats={i['name']:material(i) for i in entry['materials']}
 opt=unreal.FbxImportUI();opt.import_mesh=True;opt.import_materials=False;opt.import_textures=False;opt.import_as_skeletal=False;opt.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH;opt.static_mesh_import_data.combine_meshes=True
 imp(out/(name+'.fbx'),'/Game/Production/Meshes',name,opt);mesh=unreal.load_asset('/Game/Production/Meshes/'+name);assert mesh,name
 for i,slot in enumerate(mesh.get_editor_property('static_materials')):
  key=str(slot.material_slot_name);chosen=mats.get(key) or next(iter(mats.values()));mesh.set_material(i,chosen)
 unreal.EditorAssetLibrary.save_loaded_asset(mesh);unreal.log('PRODUCTION_MESH '+name+' bounds='+str(mesh.get_bounds()))

m=unreal.load_asset('/Game/Production/Materials/M_River')
if not m:m=a.create_asset('M_River','/Game/Production/Materials',unreal.Material,unreal.MaterialFactoryNew())
ml.delete_all_material_expressions(m)
def connect(a,o,b,i):assert ml.connect_material_expressions(a,o,b,i)
position=node(m,unreal.MaterialExpressionWorldPosition)
time=node(m,unreal.MaterialExpressionTime)
def water_custom(code):
 n=node(m,unreal.MaterialExpressionCustom)
 n.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT3)
 inputs=[]
 for key in ['P','T']:
  inp=unreal.CustomInput();inp.set_editor_property('input_name',key);inputs.append(inp)
 n.set_editor_property('inputs',inputs);n.set_editor_property('code',code)
 connect(position,'',n,'P');connect(time,'',n,'T');return n
normal=water_custom('return normalize(float3(.06*sin(P.x*.015+T*1.4)+.035*cos(P.y*.021-T),.07*cos(P.y*.016-T*1.1),1));')
wire(normal,'',m,unreal.MaterialProperty.MP_NORMAL)
color=water_custom('float q=.5+.5*sin(P.x*.001+P.y*.007-T*.3);return lerp(float3(.012,.055,.043),float3(.035,.12,.095),q);')
wire(color,'',m,unreal.MaterialProperty.MP_BASE_COLOR)
wave=water_custom('return float3(0,0,2*sin(P.x*.009+T*1.2)+1.1*sin(P.y*.016-T*.9));')
wire(wave,'',m,unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
for value,prop in [(.19,unreal.MaterialProperty.MP_ROUGHNESS),(.28,unreal.MaterialProperty.MP_METALLIC),(.7,unreal.MaterialProperty.MP_SPECULAR)]:
 wire(node(m,unreal.MaterialExpressionConstant,r=value),'',m,prop)
ml.recompile_material(m);unreal.EditorAssetLibrary.save_loaded_asset(m,False)
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True,True)
unreal.log('LANDMARKS_IMPORTED_WATER_READY')
_quit_after_notifications()
