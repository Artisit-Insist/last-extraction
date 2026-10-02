"""Fit CC0 clothing to the existing 52 bone rig; keep all earlier assets intact.
Run with native Blender --background --python this_file.py.
"""
import bpy, math, json, re
from pathlib import Path
from mathutils import Vector, Matrix
from mathutils.kdtree import KDTree

ROOT=Path(__file__).resolve().parents[2]/'ArtSource/Production'
OUT=ROOT/'Characters06';OUT.mkdir(exist_ok=True)
bpy.ops.wm.open_mainfile(filepath=str(ROOT/'CommandoRigged.blend'))
source=bpy.data.objects['CommandoRigged'];arm=bpy.data.objects['CommandoRiggedRig']
arm.animation_data_clear()
for pb in arm.pose.bones:pb.matrix_basis=Matrix.Identity(4)
bpy.context.view_layer.update()
verts=[];uv=[];faces=[];fuv=[];groups={};group=''
for line in (ROOT/'human_base/base.obj').read_text().splitlines():
 p=line.split()
 if not p:continue
 if p[0]=='v':verts.append(Vector(map(float,p[1:4])))
 elif p[0]=='vt':uv.append(tuple(map(float,p[1:3])))
 elif p[0]=='g':group=p[1]
 elif p[0]=='f':
  ids=[int(x.split('/')[0])-1 for x in p[1:]];groups.setdefault(group,set()).update(ids)
  if group=='body':faces.append(ids);fuv.append([int(x.split('/')[1])-1 for x in p[1:]])
for n in ['caucasian-male-young.target','universal-male-young-maxmuscle-averageweight.target']:
 for line in (ROOT/'human_base'/n).read_text().splitlines():
  p=line.split()
  if p and not p[0].startswith('#'):verts[int(p[0])]+=Vector(map(float,p[1:4]))
used=sorted(groups['body']);lo=min(verts[i].y for i in used);scale=1.86/(max(verts[i].y for i in used)-lo)
def cv(v):return Vector((v.x*scale,-v.z*scale,(v.y-lo)*scale))
base={i:cv(verts[i]) for i in used}
weights={i:{source.vertex_groups[g.group].name:g.weight for g in source.data.vertices[j].groups if g.weight>1e-6} for j,i in enumerate(used)}
kd=KDTree(len(used))
for i in used:kd.insert(base[i],i)
kd.balance()
def source_weights(i):return weights.get(i) or weights[kd.find(cv(verts[i]))[1]]
materials={};manifest={}
def material(name,color=(.15,.18,.1),rough=.8,texture=None,normal=None,alpha=False,ao=None):
 if name in materials:return materials[name]
 m=bpy.data.materials.new(name);m.use_nodes=True;m.diffuse_color=(*color,1)
 n=m.node_tree.nodes;l=m.node_tree.links;bs=n.get('Principled BSDF');bs.inputs['Base Color'].default_value=(*color,1);bs.inputs['Roughness'].default_value=rough
 def tex(path,noncolor=False):
  t=n.new('ShaderNodeTexImage');t.image=bpy.data.images.load(str(path),check_existing=True)
  if noncolor:t.image.colorspace_settings.name='Non-Color'
  return t
 if texture:
  t=tex(texture);l.new(t.outputs['Color'],bs.inputs['Base Color'])
  if texture.name=='Camouflage.png':
   coord=n.new('ShaderNodeTexCoord');mul=n.new('ShaderNodeVectorMath');mul.operation='SCALE';mul.inputs['Scale'].default_value=3;l.new(coord.outputs['UV'],mul.inputs[0]);l.new(mul.outputs[0],t.inputs['Vector'])
  if alpha:l.new(t.outputs['Alpha'],bs.inputs['Alpha']);m.surface_render_method='DITHERED'
 if ao and not texture:
  t=tex(ao,True);mix=n.new('ShaderNodeMixRGB');mix.blend_type='MULTIPLY';mix.inputs[0].default_value=.55;mix.inputs[1].default_value=(*color,1);l.new(t.outputs['Color'],mix.inputs[2]);l.new(mix.outputs[0],bs.inputs['Base Color'])
 if normal:
  t=tex(normal,True);nm=n.new('ShaderNodeNormalMap');nm.inputs['Strength'].default_value=.55;l.new(t.outputs['Color'],nm.inputs['Color']);l.new(nm.outputs[0],bs.inputs['Normal'])
 materials[name]=m
 manifest[name]={'color':color,'roughness':rough,'texture':str(texture.relative_to(ROOT)) if texture else None,'normal':str(normal.relative_to(ROOT)) if normal else None,'ao':str(ao.relative_to(ROOT)) if ao else None,'masked':alpha}
 return m
assets=ROOT/'MakeHumanSystemAssets'
skin=material('Role06_Skin',rough=.58,texture=assets/'skins/young_caucasian_male/young_lightskinned_male_diffuse.png')
leather=material('Role06_Leather',(.035,.032,.022),.7)
webbing=material('Role06_Webbing',(.085,.105,.046),.87)
helmetmat=material('Role06_Helmet',(.075,.11,.048),.64)
metal=material('Role06_Metal',(.043,.049,.04),.4)
eye=material('Role06_Eye',(.62,.59,.52),.24)
iris=material('Role06_Iris',(.035,.019,.008),.22)
bootsmat=material('Role06_Boots',rough=.8,texture=assets/'clothes/shoes02/shoes02_diffuse.png')

def make(name,points,polygons,uvs=None,uvfaces=None,mats=None,ws=None):
 me=bpy.data.meshes.new(name);me.from_pydata(points,[],polygons);me.update()
 ob=bpy.data.objects.new(name,me);bpy.context.collection.objects.link(ob)
 for m in mats or []:me.materials.append(m)
 for f in me.polygons:f.use_smooth=True
 if uvs:
  layer=me.uv_layers.new(name='UVMap')
  for f,fu in zip(me.polygons,uvfaces):
   for li,ui in zip(f.loop_indices,fu):layer.data[li].uv=uvs[ui]
 if ws:
  vg={n:ob.vertex_groups.new(name=n) for n in arm.data.bones.keys()}
  for j,w in enumerate(ws):
   total=sum(w.values())
   for n,v in w.items():
    if v>0:vg[n].add([j],v/total,'REPLACE')
 return ob
def bind(ob,bone):
 ob.vertex_groups.new(name=bone).add(list(range(len(ob.data.vertices))),1,'REPLACE');return ob
def fit(folder,name,mat):
 folder=assets/folder;refs=[];scaling=Vector((1,1,1));deleted=set();in_delete=False
 for line in (folder/(name+'.mhclo')).read_text().splitlines():
  p=line.split()
  if not p or p[0].startswith('#'):continue
  if p[0]=='delete_verts':in_delete=True;continue
  if in_delete:
   for a,b in re.findall(r'(\d+)(?:\s*-\s*(\d+))?',line):deleted.update(range(int(a),int(b or a)+1))
  elif p[0] in ['x_scale','y_scale','z_scale']:
   k={'x_scale':0,'y_scale':1,'z_scale':2}[p[0]];scaling[k]=abs(verts[int(p[1])][k]-verts[int(p[2])][k])/float(p[3])
  elif len(p)==9 and all(t.lstrip('-').isdigit() for t in p[:3]):refs.append(([int(n) for n in p[:3]],[float(n) for n in p[3:6]],Vector(map(float,p[6:9]))))
 points=[];ws=[]
 for ids,coef,offset in refs:
  points.append(cv(sum((verts[i]*w for i,w in zip(ids,coef)),Vector())+Vector(tuple(offset[k]*scaling[k] for k in range(3)))))
  w={}
  for i,c in zip(ids,coef):
   for n,v in source_weights(i).items():w[n]=w.get(n,0)+max(0,c)*v
  ws.append(w)
 uvs=[];fs=[];fu=[];expected=0
 for line in (folder/(name+'.obj')).read_text().splitlines():
  p=line.split()
  if not p:continue
  if p[0]=='v':expected+=1
  elif p[0]=='vt':uvs.append(tuple(map(float,p[1:3])))
  elif p[0]=='f':fs.append([int(x.split('/')[0])-1 for x in p[1:]]);fu.append([int(x.split('/')[1])-1 for x in p[1:]])
 assert len(points)==expected,(name,len(points),expected)
 return make(name,points,fs,uvs,fu,[mat],ws),deleted
def sphere(name,p,size,mat,bone='head'):
 bpy.ops.mesh.primitive_uv_sphere_add(segments=24,ring_count=16,location=p);o=bpy.context.object;o.name=name;o.scale=size;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);o.data.materials.append(mat)
 for f in o.data.polygons:f.use_smooth=True
 return bind(o,bone)
def box(name,p,size,mat,bone='spine',bevel=.009):
 bpy.ops.mesh.primitive_cube_add(size=1,location=p);o=bpy.context.object;o.name=name;o.scale=size;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);o.data.materials.append(mat)
 b=o.modifiers.new('Soft sewn edges','BEVEL');b.width=bevel;b.segments=3;bpy.ops.object.modifier_apply(modifier=b.name)
 return bind(o,bone)
def shell(name,rings,mat,bone='head',thickness=.003):
 # Rings contain z, x radius, y radius, y centre. Closed quads give soft curved silhouettes.
 n=48;p=[(rx*math.cos(i*math.tau/n),cy+ry*math.sin(i*math.tau/n),z) for z,rx,ry,cy in rings for i in range(n)]
 fs=[(j*n+i,j*n+(i+1)%n,(j+1)*n+(i+1)%n,(j+1)*n+i) for j in range(len(rings)-1) for i in range(n)]
 o=make(name,p,fs,mats=[mat]);bind(o,bone)
 mod=o.modifiers.new('Fabric thickness','SOLIDIFY');mod.thickness=thickness;bpy.context.view_layer.objects.active=o;bpy.ops.object.modifier_apply(modifier=mod.name)
 return o
def strap(name,points,width,mat,bone='chest'):
 p=[(x+side*width/2,y,z) for x,y,z in points for side in [-1,1]]
 o=make(name,p,[(j*2,j*2+1,j*2+3,j*2+2) for j in range(len(points)-1)],mats=[mat]);bind(o,bone)
 mod=o.modifiers.new('Canvas thickness','SOLIDIFY');mod.thickness=.004;bpy.context.view_layer.objects.active=o;bpy.ops.object.modifier_apply(modifier=mod.name)
 return o
def tube(name,points,r,mat,bone='head'):
 c=bpy.data.curves.new(name,'CURVE');c.dimensions='3D';c.bevel_depth=r;c.bevel_resolution=2;s=c.splines.new('POLY');s.points.add(len(points)-1)
 for v,p in zip(s.points,points):v.co=(*p,1)
 o=bpy.data.objects.new(name,c);bpy.context.collection.objects.link(o);c.materials.append(mat);bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o;bpy.ops.object.convert(target='MESH');return bind(bpy.context.object,bone)

roles=[('Guard06','male_casualsuit02','helmet'),('Scout06','male_casualsuit02','boonie'),('Heavy06','male_casualsuit02','armor'),('Civilian06','male_casualsuit06','civilian'),('Worker06','male_worksuit01','worker')]
allroles=[];report={}
for role,cloth,kind in roles:
 parts=[];folder=assets/'clothes'/cloth;normal=folder/(cloth+'_normal.png');ao=folder/(cloth+'_ao.png')
 color=(.16,.2,.075) if kind=='helmet' else (.13,.16,.10) if kind=='boonie' else (.095,.13,.07) if kind=='armor' else (.31,.29,.21)
 cm=material('Role06_'+role+'_Cloth',color,.9,texture=folder/(cloth+'_diffuse.png') if kind=='worker' else ROOT/'Camouflage.png' if kind in ['helmet','armor'] else None,normal=normal if normal.exists() else None,ao=ao if ao.exists() else None)
 clothing,hidden=fit('clothes/'+cloth,cloth,cm);parts.append(clothing)
 if kind=='civilian':
  denim=material('Role06_Denim',(.027,.07,.1),.88,normal=normal if normal.exists() else None,ao=ao if ao.exists() else None);clothing.data.materials.append(denim)
  for f in clothing.data.polygons:
   if sum(clothing.data.vertices[i].co.z for i in f.vertices)/len(f.vertices)<1.03:f.material_index=1
 boots,foot_hidden=fit('clothes/shoes02','shoes02',bootsmat);hidden|=foot_hidden;parts.append(boots)
 # Use the clothing's authored body mask; retain uncovered neck, hands, forearms and face.
 keep=[k for k,f in enumerate(faces) if not all(i in hidden for i in f) and not all(base[i].z<.20 for i in f)]
 bodyids=sorted({i for k in keep for i in faces[k]});mapping={i:j for j,i in enumerate(bodyids)}
 body=make(role,[base[i] for i in bodyids],[[mapping[i] for i in faces[k]] for k in keep],uv,[fuv[k] for k in keep],[skin],[weights[i] for i in bodyids]);parts.append(body)
 hairname='short01' if kind=='worker' else 'short03' if kind=='civilian' else 'short02'
 hairmat=material('Role06_'+hairname,rough=.85,texture=assets/'hair'/hairname/(hairname+'_diffuse.png'),alpha=True)
 if kind in ['civilian','worker']:
  hair,_=fit('hair/'+hairname,hairname,hairmat);parts.append(hair)
 browsmat=material('Role06_Brows',rough=.9,texture=assets/'eyebrows/eyebrow003/eyebrow003.png',alpha=True)
 brows,_=fit('eyebrows/eyebrow003','eyebrow003',browsmat);parts.append(brows)
 for side in ['l','r']:
  ids=groups['joint-'+side+'-eye'];p=cv(sum((verts[i] for i in ids),Vector())/len(ids))
  parts.append(sphere('Eye',p,(.013,.010,.010),eye));parts.append(sphere('Iris',p+Vector((0,-.009,0)),(.005,.003,.006),iris))
 if kind in ['helmet','armor']:
  parts.append(shell('Steel helmet',[(1.735,.112,.135,-.064),(1.765,.116,.138,-.064),(1.81,.104,.129,-.064),(1.86,.075,.101,-.064),(1.881,.052,.068,-.064),(1.894,.024,.032,-.064),(1.899,.002,.003,-.064)],helmetmat))
  parts.append(shell('Helmet rolled rim',[(1.735,.116,.141,-.064),(1.742,.117,.142,-.064)],helmetmat,thickness=.005))
  for s in [-1,1]:parts.append(tube('Chin webbing',[(s*.105,-.01,1.747),(s*.075,-.07,1.65),(s*.038,-.11,1.637)],.005,webbing))
 elif kind=='boonie':
  parts.append(shell('Boonie brim',[(1.752,.153,.181,-.064),(1.766,.11,.139,-.064)],webbing,thickness=.005))
  parts.append(shell('Boonie crown',[(1.765,.113,.144,-.064),(1.87,.094,.118,-.064),(1.883,.005,.005,-.064)],webbing))
 if kind in ['helmet','boonie','armor']:
  parts.append(shell('Utility belt',[(1.005,.193,.12,0),(1.048,.19,.119,0)],leather,'pelvis'))
  for s in [-1,1]:
   parts.append(strap('Shoulder webbing',[(s*.12,.115,1.24),(s*.12,.115,1.45),(s*.12,.015,1.53),(s*.13,-.17,1.43),(s*.13,-.166,1.12)],.05,webbing,'chest'))
   for x in [.07,.145]:
    parts.append(box('Rounded magazine pouch',(s*x,-.143,1.13),(.068,.065,.15),webbing))
    parts.append(box('Pouch flap',(s*x,-.18,1.177),(.073,.012,.05),webbing,bevel=.004))
   parts.append(sphere('Canteen',(s*.2,.055,1.03),(.045,.044,.075),webbing,'pelvis'))
  if kind=='armor':
   parts.append(shell('Flak jacket',[(1.1,.23,.164,0),(1.24,.254,.187,0),(1.43,.264,.196,0),(1.49,.18,.159,0)],webbing,'chest',.008))
   for z in [1.19,1.26,1.33,1.4]:parts.append(tube('Vest seams',[(-.18,-.14,z),(-.1,-.184,z),(0,-.199,z),(.1,-.184,z),(.18,-.14,z)],.0025,leather,'chest'))
 bpy.ops.object.select_all(action='DESELECT')
 for o in parts:o.select_set(True)
 bpy.context.view_layer.objects.active=body;bpy.ops.object.join();body.name=role;body.parent=arm;mod=body.modifiers.new('Existing 52 bone rig','ARMATURE');mod.object=arm
 assert all(sum(g.weight for g in v.groups)>.99 for v in body.data.vertices),role+' has unweighted vertices'
 bpy.ops.object.select_all(action='DESELECT');body.select_set(True);arm.select_set(True);bpy.context.view_layer.objects.active=arm
 bpy.ops.export_scene.fbx(filepath=str(OUT/(role+'.fbx')),use_selection=True,object_types={'MESH','ARMATURE'},add_leaf_bones=False,axis_forward='-Y',axis_up='Z',bake_anim=False)
 report[role]={'vertices':len(body.data.vertices),'polygons':len(body.data.polygons),'bones':len(arm.data.bones),'materials':[m.name for m in body.data.materials]}
 allroles.append(body);body.hide_render=True;body.hide_set(True)
 print('ROLE06_EXPORTED',role,report[role],flush=True)
source.hide_render=True;source.hide_set(True)
arm.animation_data_create();arm.animation_data.action=bpy.data.actions['Idle'];arm.animation_data.action_slot=arm.animation_data.action.slots[0];bpy.context.scene.frame_set(1)
allroles[0].hide_render=False;allroles[0].hide_set(False)
(OUT/'materials.json').write_text(json.dumps(manifest,indent=2));(OUT/'manifest.json').write_text(json.dumps(report,indent=2))
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'CharacterRoles06.blend'))
print('CHARACTER_ROLES06_READY',flush=True)
