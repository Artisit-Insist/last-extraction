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

for filename in ['world_revision_manifest.json','regional_props_manifest.json','building_variants_manifest.json']:
 manifest=json.loads((out/filename).read_text())
 for name,entry in manifest.items():
  mats={i['name']:material(i) for i in entry['materials']}
  opt=unreal.FbxImportUI();opt.import_mesh=True;opt.import_materials=False;opt.import_textures=False;opt.import_as_skeletal=False;opt.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH;opt.static_mesh_import_data.combine_meshes=True
  imp(out/(name+'.fbx'),'/Game/Production/Meshes',name,opt);mesh=unreal.load_asset('/Game/Production/Meshes/'+name);assert mesh,name
  for i,slot in enumerate(mesh.get_editor_property('static_materials')):mesh.set_material(i,mats.get(str(slot.material_slot_name)) or next(iter(mats.values())))
  if name in ['FieldHangar','ConcreteBunker','FortressWall','RiverDock']:
   mesh.get_editor_property('body_setup').set_editor_property('collision_trace_flag',unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
  unreal.EditorAssetLibrary.save_loaded_asset(mesh);unreal.log('REGIONAL_ASSET '+name)

def connect(a,o,b,i):assert ml.connect_material_expressions(a,o,b,i),(str(a),o,str(b),i)
def fresh(name):
 m=unreal.load_asset('/Game/Production/Materials/'+name)
 if not m:m=a.create_asset(name,'/Game/Production/Materials',unreal.Material,unreal.MaterialFactoryNew())
 ml.delete_all_material_expressions(m);return m

def custom(m,code,output=unreal.CustomMaterialOutputType.CMOT_FLOAT3,inputs=None):
 n=node(m,unreal.MaterialExpressionCustom);n.set_editor_property('output_type',output);ci=[]
 for key in inputs:
  i=unreal.CustomInput();i.set_editor_property('input_name',key);ci.append(i)
 n.set_editor_property('inputs',ci);n.set_editor_property('code',code)
 for key,(src,pin) in inputs.items():connect(src,pin,n,key)
 return n
m=fresh('M_River');pos=node(m,unreal.MaterialExpressionWorldPosition);time=node(m,unreal.MaterialExpressionTime);vc=node(m,unreal.MaterialExpressionVertexColor)
normal=custom(m,'float a=P.x*.073+P.y*.024-T*1.05; float b=P.x*-.034+P.y*.091-T*1.42; float c=P.x*.21+P.y*.16-T*2.1; return normalize(float3(.018*sin(a)+.011*cos(b)+.007*sin(c),.018*cos(b)+.009*sin(a*.87)+.004*cos(c),1));',inputs={'P':(pos,''),'T':(time,'')});wire(normal,'',m,unreal.MaterialProperty.MP_NORMAL)
color=custom(m,'float fleck=.5+.5*sin(P.y*.025-P.x*.01-T*.55)*sin(P.x*.041+T*.16); float edge=smoothstep(.48,.95,Shore); return lerp(float3(.015,.043,.034),float3(.070,.083,.047),edge)+fleck*.003;',inputs={'P':(pos,''),'T':(time,''),'Shore':(vc,'R')});wire(color,'',m,unreal.MaterialProperty.MP_BASE_COLOR)
for v,pr in [(.31,unreal.MaterialProperty.MP_ROUGHNESS),(0,unreal.MaterialProperty.MP_METALLIC),(.32,unreal.MaterialProperty.MP_SPECULAR)]:wire(node(m,unreal.MaterialExpressionConstant,r=v),'',m,pr)
ml.recompile_material(m);unreal.EditorAssetLibrary.save_loaded_asset(m,False)

for name,is_road in [('M_GroundRegional',False),('M_Road',True)]:
 m=fresh(name);pos=node(m,unreal.MaterialExpressionWorldPosition);uv=custom(m,'return P.xy*.0025;',unreal.CustomMaterialOutputType.CMOT_FLOAT2,{'P':(pos,'')});vc=node(m,unreal.MaterialExpressionVertexColor)
 diff=node(m,unreal.MaterialExpressionTextureSample,texture=unreal.load_asset('/Game/Production/Textures/brown_mud_leaves_01_diff_2k'));connect(uv,'',diff,'UVs')
 terrain_type=node(m,unreal.MaterialExpressionScalarParameter,parameter_name='TerrainType',default_value=0)
 shade=custom(m,('float grain=dot(D,float3(.3,.59,.11));return Tint*(.72+grain*1.8);' if is_road else 'float gray=dot(D,float3(.30,.59,.11));float3 natural=D*Tint*1.2;float3 road=gray*float3(.46,.31,.18);if(Biome>2.5&&Biome<3.5){natural=gray*float3(.69,.64,.55);road=gray*float3(.41,.38,.32);}if(Biome>3.5&&Biome<4.5){natural=lerp(D,gray.xxx,.65)*Tint;road=gray*float3(.40,.40,.36);}if(Biome>4.5){natural=lerp(D,gray.xxx,.40)*Tint;road=float3(.048,.054,.05)*(.8+gray*.7);}return lerp(natural,road,Path);'),inputs={'D':(diff,'RGB'),'Tint':(vc,''),'Path':(vc,'A'),'Biome':(terrain_type,'')});wire(shade,'',m,unreal.MaterialProperty.MP_BASE_COLOR)

 normal=node(m,unreal.MaterialExpressionTextureSample,texture=unreal.load_asset('/Game/Production/Textures/brown_mud_leaves_01_nor_gl_2k'));normal.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL);connect(uv,'',normal,'UVs');wire(normal,'RGB',m,unreal.MaterialProperty.MP_NORMAL)
 wire(node(m,unreal.MaterialExpressionConstant,r=.94),'',m,unreal.MaterialProperty.MP_ROUGHNESS);ml.recompile_material(m);unreal.EditorAssetLibrary.save_loaded_asset(m,False)

# Add subtle spatial weathering to newly authored concrete and painted metal.
for name in ['Fortress_Concrete','Weathered_Iron','Transport_Olive','Hangar_Galvanized','Mine_Rust','Fuel_Silver']:
 m=unreal.load_asset('/Game/Production/Materials/M_'+name)
 if not m:continue
 pos=node(m,unreal.MaterialExpressionWorldPosition)
 tint={'Fortress_Concrete':(.26,.255,.215),'Weathered_Iron':(.12,.15,.13),'Transport_Olive':(.085,.11,.065),'Hangar_Galvanized':(.22,.245,.24),'Mine_Rust':(.22,.095,.03),'Fuel_Silver':(.35,.36,.32)}[name]
 base=node(m,unreal.MaterialExpressionConstant3Vector,constant=unreal.LinearColor(*tint,1))
 shade=custom(m,'float h=frac(sin(dot(floor(P*1.4),float3(12.9898,78.233,34.72)))*43758.5453); float stain=.85+.09*sin(P.x*.032+sin(P.z*.027))+.04*sin(P.y*.2);return C*(stain+h*.12);',inputs={'P':(pos,''),'C':(base,'')});wire(shade,'',m,unreal.MaterialProperty.MP_BASE_COLOR)
 ml.recompile_material(m);unreal.EditorAssetLibrary.save_loaded_asset(m,False)

skin=root/'MakeHumanSystemAssets/skins/young_caucasian_male/young_lightskinned_male_diffuse.png'
t=tex(skin);m=unreal.load_asset('/Game/Production/Materials/M_Commando_Skin')
if m:
 sam=node(m,unreal.MaterialExpressionTextureSample,texture=t);wire(sam,'RGB',m,unreal.MaterialProperty.MP_BASE_COLOR);wire(node(m,unreal.MaterialExpressionConstant,r=.64),'',m,unreal.MaterialProperty.MP_ROUGHNESS);ml.recompile_material(m);unreal.EditorAssetLibrary.save_loaded_asset(m,False)
imp(root/'RescueRotor.wav','/Game/Audio','RescueRotor');snd=unreal.load_asset('/Game/Audio/RescueRotor');snd.set_editor_property('looping',True);unreal.EditorAssetLibrary.save_loaded_asset(snd)
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True,True);unreal.log('WORLD_REVISION_IMPORTED');_quit_after_notifications()
