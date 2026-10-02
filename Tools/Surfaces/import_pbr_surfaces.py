"""Add scanned PBR surfaces, retaining the 0.3 material assets for comparison."""
from pathlib import Path
import json,sys,traceback
import unreal
root=Path(unreal.Paths.project_dir()).resolve()
sys.path.insert(0,str(root/'Tools'))
from editor_shutdown import quit_after_notifications
ml=unreal.MaterialEditingLibrary;at=unreal.AssetToolsHelpers.get_asset_tools()
source=root/'ArtSource/Production/Surface04'
manifest=json.loads((source/'source_manifest.json').read_text())
textures={};made={}
def node(m,cls,**props):
 n=ml.create_material_expression(m,cls)
 for k,v in props.items():n.set_editor_property(k,v)
 return n
def link(a,ap,b,bp):assert ml.connect_material_expressions(a,ap,b,bp),(a.get_name(),ap,b.get_name(),bp)
def prop(n,m,p,pin=''):assert ml.connect_material_property(n,pin,p),p
def custom(m,code,inputs,dim=3):
 n=node(m,unreal.MaterialExpressionCustom);n.set_editor_property('output_type',{1:unreal.CustomMaterialOutputType.CMOT_FLOAT1,2:unreal.CustomMaterialOutputType.CMOT_FLOAT2,3:unreal.CustomMaterialOutputType.CMOT_FLOAT3}[dim]);ci=[]
 for key in inputs:
  p=unreal.CustomInput();p.set_editor_property('input_name',key);ci.append(p)
 n.set_editor_property('inputs',ci);n.set_editor_property('code',code)
 for key,val in inputs.items():link(val[0],val[1],n,key)
 return n
def fresh(name):
 path='/Game/Production/Materials04/M04_'+name;m=unreal.load_asset(path)
 if not m:m=at.create_asset('M04_'+name,'/Game/Production/Materials04',unreal.Material,unreal.MaterialFactoryNew())
 # Iterate a snapshot: UE 5.8 DeleteAllMaterialExpressions mutates its own range.
 for expr in list(ml.get_material_expressions(m)):ml.delete_material_expression(m,expr)
 assert ml.get_num_material_expressions(m)==0,name
 return m
def constant(m,v):return node(m,unreal.MaterialExpressionConstant,r=v)
def vector(m,v):return node(m,unreal.MaterialExpressionConstant3Vector,constant=unreal.LinearColor(*v,1))
def sample(m,texture,role,uv):
 n=node(m,unreal.MaterialExpressionTextureSample,texture=texture,sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL if role=='nor_dx' else unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR if role=='arm' else unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
 link(uv,'',n,'UVs');return n

def import_textures():
 for name,entry in manifest.items():
  textures[name]={}
  for role,info in entry['maps'].items():
   f=source/info['file'];path='/Game/Production/Textures04/'+f.stem;t=unreal.load_asset(path)
   if not t:
    task=unreal.AssetImportTask();task.filename=str(f);task.destination_path='/Game/Production/Textures04';task.destination_name=f.stem;task.automated=True;task.save=True;task.replace_existing=False;at.import_asset_tasks([task]);t=unreal.load_asset(path)
   assert t,path
   if role=='nor_dx':t.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_NORMALMAP);t.set_editor_property('flip_green_channel',False)
   t.set_editor_property('srgb',role=='Diffuse');t.set_editor_property('never_stream',False);unreal.EditorAssetLibrary.save_loaded_asset(t,False);textures[name][role]=t

def build_surface(name,scan,tile,tint,desat=0,metal=0,strength=.65):
 m=fresh(name);m.set_editor_property('tangent_space_normal',False)
 pos=node(m,unreal.MaterialExpressionWorldPosition)
 loc=node(m,unreal.MaterialExpressionTransformPosition,transform_source_type=unreal.MaterialPositionTransformSource.TRANSFORMPOSSOURCE_WORLD,transform_type=unreal.MaterialPositionTransformSource.TRANSFORMPOSSOURCE_LOCAL);link(pos,'',loc,'')
 vn=node(m,unreal.MaterialExpressionVertexNormalWS)
 normal=node(m,unreal.MaterialExpressionTransform,transform_source_type=unreal.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_WORLD,transform_type=unreal.MaterialVectorCoordTransform.TRANSFORM_LOCAL);link(vn,'',normal,'')
 coords=[]
 for axis in ['yz','xz','xy']:coords.append(custom(m,f'return P.{axis}/{tile:.6f};',{'P':(loc,'')},2))
 channels={}
 for role in ['Diffuse','nor_dx','arm']:
  samples=[sample(m,textures[scan][role],role,uv) for uv in coords]
  inp={'N':(normal,''),'X':(samples[0],'RGB'),'Y':(samples[1],'RGB'),'Z':(samples[2],'RGB')}
  code='float3 w=pow(abs(N),8);w/=max(w.x+w.y+w.z,0.0001);'
  if role=='nor_dx':
   code+=f'X.xy*={strength};Y.xy*={strength};Z.xy*={strength};'
   code+='float3 nx=float3(abs(X.z)*N.x,X.x+N.y,X.y+N.z);float3 ny=float3(Y.xy+N.xz,abs(Y.z)*N.y).xzy;float3 nz=float3(Z.xy+N.xy,abs(Z.z)*N.z);return normalize(nx*w.x+ny*w.y+nz*w.z);'
  else:code+='return X*w.x+Y*w.y+Z*w.z;'
  channels[role]=custom(m,code,inp)
 tint_node=node(m,unreal.MaterialExpressionVectorParameter,parameter_name='Tint',default_value=unreal.LinearColor(*tint,1))
 col=custom(m,f'float gray=dot(D,float3(.3,.59,.11));return lerp(D,gray.xxx,{desat})*Tint;',{'D':(channels['Diffuse'],''),'Tint':(tint_node,'')});prop(col,m,unreal.MaterialProperty.MP_BASE_COLOR)
 worldnormal=node(m,unreal.MaterialExpressionTransform,transform_source_type=unreal.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_LOCAL,transform_type=unreal.MaterialVectorCoordTransform.TRANSFORM_WORLD);link(channels['nor_dx'],'',worldnormal,'');prop(worldnormal,m,unreal.MaterialProperty.MP_NORMAL)
 for code,mp in [('return lerp(.72,1,A.r);',unreal.MaterialProperty.MP_AMBIENT_OCCLUSION),('return clamp(A.g,.38,.98);',unreal.MaterialProperty.MP_ROUGHNESS)]:prop(custom(m,code,{'A':(channels['arm'],'')},1),m,mp)
 prop(constant(m,metal),m,unreal.MaterialProperty.MP_METALLIC)
 ml.recompile_material(m);unreal.EditorAssetLibrary.save_loaded_asset(m,False);made[name]=m;unreal.log('PBR_SURFACE '+name)

def ground_set(name):
 if name=='forest':return {'Diffuse':unreal.load_asset('/Game/Production/Textures/brown_mud_leaves_01_diff_2k'),'nor_dx':unreal.load_asset('/Game/Production/Textures/brown_mud_leaves_01_nor_gl_2k'),'arm':None}
 return textures[name]
def build_ground(chapter,natural,road,tile,roadtile,tint):
 m=fresh('Ground_'+str(chapter));pos=node(m,unreal.MaterialExpressionWorldPosition);vc=node(m,unreal.MaterialExpressionVertexColor)
 uv=custom(m,f'return P.xy/{tile};',{'P':(pos,'')},2);ruv=custom(m,f'return P.xy/{roadtile};',{'P':(pos,'')},2)
 a=ground_set(natural);b=ground_set(road)
 outputs={}
 for role in ['Diffuse','nor_dx','arm']:
  def de_tile(tex,coords):
   if not tex:return vector(m,(1,.94,0))
   first=sample(m,tex,role,coords)
   alt=custom(m,'return float2(U.x*.819-U.y*.573,U.x*.573+U.y*.819)*1.37+float2(7.13,3.71);',{'U':(coords,'')},2)
   second=sample(m,tex,role,alt)
   code='float k=.30+.12*sin(P.x*.0013)*cos(P.y*.0017);'
   if role=='nor_dx':code+='B.xy=float2(B.x*.819+B.y*.573,-B.x*.573+B.y*.819);return normalize(lerp(A,B,k));'
   else:code+='return lerp(A,B,k);'
   return custom(m,code,{'A':(first,'RGB'),'B':(second,'RGB'),'P':(pos,'')})
  na=de_tile(a[role],uv);nb=de_tile(b[role],ruv)
  outputs[role]=(na,nb)
 road_gain=.32 if chapter==1 else .52 if road=='asphalt_02' else .74
 path_node=vc;path_pin='A'
 if chapter==1:
  path_node=custom(m,'return max(Path,1-smoothstep(-125,45,P.z));',{'P':(pos,''),'Path':(vc,'A')},1);path_pin=''
 na,nb=outputs['Diffuse'];col=custom(m,f'float3 tint=float3({tint[0]},{tint[1]},{tint[2]});return lerp(A*tint,B*{road_gain},saturate(Path));',{'A':(na,''),'B':(nb,''),'Path':(path_node,path_pin)});prop(col,m,unreal.MaterialProperty.MP_BASE_COLOR)
 na,nb=outputs['nor_dx'];norm=custom(m,'A.xy*=.7;B.xy*=.6;return normalize(lerp(A,B,saturate(Path)));',{'A':(na,''),'B':(nb,''),'Path':(path_node,path_pin)});prop(norm,m,unreal.MaterialProperty.MP_NORMAL)
 na,nb=outputs['arm'];rough=custom(m,'return clamp(lerp(A.g,B.g,Path),.6,.98);',{'A':(na,''),'B':(nb,''),'Path':(path_node,path_pin)},1);prop(rough,m,unreal.MaterialProperty.MP_ROUGHNESS)
 ml.recompile_material(m);unreal.EditorAssetLibrary.save_loaded_asset(m,False);unreal.log('PBR_GROUND '+str(chapter))

def build_water():
 m=fresh('River');m.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_SINGLE_LAYER_WATER)
 pos=node(m,unreal.MaterialExpressionWorldPosition);time=node(m,unreal.MaterialExpressionTime)
 norm=custom(m,'float a=P.x*.021+P.y*.049+sin(P.x*.003+P.y*.002)*1.3-T*.8;float b=P.x*-.071+P.y*.034+sin(P.x*.004-P.y*.003)*2.2-T*1.1;float c=P.x*.16+P.y*.117-T*.7;return normalize(float3(.006*sin(a)+.004*sin(b)+.002*sin(c),.007*cos(b)+.004*cos(a)+.002*cos(c),1));',{'P':(pos,''),'T':(time,'')});prop(norm,m,unreal.MaterialProperty.MP_NORMAL)
 prop(vector(m,(.011,.026,.019)),m,unreal.MaterialProperty.MP_BASE_COLOR)
 for v,mp in [(.38,unreal.MaterialProperty.MP_ROUGHNESS),(.24,unreal.MaterialProperty.MP_SPECULAR),(.08,unreal.MaterialProperty.MP_OPACITY)]:prop(constant(m,v),m,mp)
 water=node(m,unreal.MaterialExpressionSingleLayerWaterMaterialOutput)
 for inp,v in [('ScatteringCoefficients',(.0018,.0032,.0021)),('AbsorptionCoefficients',(.018,.008,.010)),('ColorScaleBehindWater',(.83,.88,.75))]:link(vector(m,v),'',water,inp)
 link(constant(m,.15),'',water,'PhaseG')
 ml.recompile_material(m);unreal.EditorAssetLibrary.save_loaded_asset(m,False);unreal.log('PBR_WATER_COMPLETE')

def build_boots():
 m=fresh('CombatBoots');m.set_editor_property('used_with_skeletal_mesh',True)
 t=unreal.load_asset('/Game/Production/Textures/shoes02_diffuse')
 sample_node=node(m,unreal.MaterialExpressionTextureSample,texture=t)
 col=custom(m,'float l=dot(C,float3(.3,.59,.11));return (l*.36+.008)*float3(.42,.38,.30);',{'C':(sample_node,'RGB')});prop(col,m,unreal.MaterialProperty.MP_BASE_COLOR)
 prop(constant(m,.84),m,unreal.MaterialProperty.MP_ROUGHNESS)
 ml.recompile_material(m);unreal.EditorAssetLibrary.save_loaded_asset(m,False)
 mesh=unreal.load_asset('/Game/Production/Rigged/CommandoVisual03');slots=mesh.get_editor_property('materials')
 for i,slot in enumerate(slots):
  if 'Boots' in str(slot.material_slot_name):slot.material_interface=m;slots[i]=slot
 mesh.set_editor_property('materials',slots);unreal.EditorAssetLibrary.save_loaded_asset(mesh,False);unreal.log('PBR_BOOTS_COMPLETE')

def main():
 import_textures()
 profiles={
 'Fortress_Concrete':('concrete_wall_006',260,(1.15,1.15,1.09),.15,0,.85),
 'Village_Plaster':('rough_plaster_brick_02',250,(1,1,1),0,0,.8),
 'Well_Stone':('concrete_wall_006',160,(.85,.9,.84),.3,0,.9),
 'Weathered_Iron':('green_metal_rust',190,(.62,.73,.62),.92,.12,.65),
 'Field_Steel':('green_metal_rust',95,(.31,.35,.32),1,.4,.35),
 'Mission_Steel':('green_metal_rust',160,(.70,.74,.72),1,.3,.45),
 'Mission_Olive':('green_metal_rust',190,(.45,.55,.31),1,0,.45),
 'Radar_Olive':('green_metal_rust',220,(.54,.68,.38),1,0,.45),
 'Transport_Olive':('green_metal_rust',340,(.46,.55,.32),1,0,.23),
 'Transport_Edge':('green_metal_rust',300,(.38,.43,.33),1,.3,.25),
 'Hangar_Galvanized':('worn_corrugated_iron',250,(.85,.90,.86),.65,.55,.65),
 'Shed_RustedRoof':('worn_corrugated_iron',240,(.73,.56,.37),.1,.12,.75),
 'Mine_Rust':('rusty_metal_03',140,(.9,.70,.40),0,.15,.65),
 'Fuel_Silver':('green_metal_rust',290,(2.2,2.25,2.15),1,.7,.3),
 'Fuel_Red':('green_metal_rust',140,(1,.55,.35),.15,0,.45),
 'Bridge_Red':('green_metal_rust',250,(.7,.35,.23),0,0,.5),
 'Crane_Ochre':('green_metal_rust',240,(2.9,1.85,.35),1,0,.5),
 'Equipment_Yellow':('green_metal_rust',160,(3.5,2.4,.4),1,0,.4),
 'Field_Sack':('denim_fabric_04',70,(2.3,1.9,1.1),1,0,.45),
 'Canvas_Tan':('denim_fabric_04',95,(2.5,2.1,1.4),1,0,.35),
 'Transport_Canvas':('denim_fabric_04',65,(1.0,1.2,.75),1,0,.35),
 'Washed_Cloth_Blue':('denim_fabric_04',85,(.7,.95,1.1),0,0,.4),
 'Washed_Cloth_Cream':('denim_fabric_04',85,(2.8,2.6,2.1),1,0,.4),
 }
 for name,p in profiles.items():build_surface(name,*p)
 bindings={}
 for asset in unreal.EditorAssetLibrary.list_assets('/Game/Production/Meshes',recursive=True,include_folder=False):
  mesh=unreal.load_asset(asset)
  if not isinstance(mesh,unreal.StaticMesh):continue
  before=[];changed=0
  for i,slot in enumerate(mesh.get_editor_property('static_materials')):
   old=slot.material_interface;before.append(old.get_path_name() if old else None)
   for name,m in made.items():
    if old and old.get_name() in ['M_'+name,'M04_'+name]:mesh.set_material(i,m);changed+=1;break
  if changed:bindings[mesh.get_name()]=before;unreal.EditorAssetLibrary.save_loaded_asset(mesh,False)
 bindings_path=source/'replaced_material_bindings.json'
 if not bindings_path.exists():bindings_path.write_text(json.dumps(bindings,indent=2))
 grounds=[('forest','dirt_floor',440,260,(.85,.98,.83)),('forest','dirt_floor',440,270,(.90,.96,.86)),('forest','dirt_floor',430,220,(.96,.90,.72)),('gravel_ground_01','dirt_floor',480,280,(.79,.78,.73)),('rocky_terrain_02','asphalt_02',500,380,(.81,.83,.79)),('dirt_floor','asphalt_02',420,400,(.82,.82,.75))]
 for i,p in enumerate(grounds):build_ground(i,*p)
 build_water();build_boots()
 unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True,True);unreal.log('PBR_SURFACES_COMPLETE materials='+str(len(made))+' meshes='+str(len(bindings)))
try:main()
except Exception:unreal.log_error(traceback.format_exc())
finally:quit_after_notifications()
