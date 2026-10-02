import bpy,math,json,random
from pathlib import Path
from mathutils import Vector
random.seed(1985)
bpy.ops.wm.read_factory_settings(use_empty=True)
root=Path(__file__).resolve().parents[2]/'ArtSource/Production';src=root/'human_base';out=root/'Export';out.mkdir(exist_ok=True)
verts=[];uvs=[];faces=[];fuv=[];groups={};group=''
for line in (src/'base.obj').read_text().splitlines():
 s=line.split()
 if not s:continue
 if s[0]=='v':verts.append(Vector(map(float,s[1:4])))
 elif s[0]=='vt':uvs.append(tuple(map(float,s[1:3])))
 elif s[0]=='g':group=s[1]
 elif s[0]=='f':
  ids=[int(v.split('/')[0])-1 for v in s[1:]];groups.setdefault(group,set()).update(ids)
  if group=='body':faces.append(ids);fuv.append([int(v.split('/')[1])-1 for v in s[1:]])
for fname in ['caucasian-male-young.target','universal-male-young-maxmuscle-averageweight.target']:
 for l in (src/fname).read_text().splitlines():
  if not l or l.startswith('#'):continue
  a=l.split();verts[int(a[0])]+=Vector(map(float,a[1:4]))
zmin=min(verts[i].y for i in groups['body']);zmax=max(verts[i].y for i in groups['body']);scale=1.86/(zmax-zmin)
def cv(v):return Vector((v.x*scale,-v.z*scale,(v.y-zmin)*scale))
joints={k:cv(sum((verts[i] for i in ix),Vector())/len(ix)) for k,ix in groups.items() if k.startswith('joint-')}
used=sorted(groups['body']);mapping={v:i for i,v in enumerate(used)}
me=bpy.data.meshes.new('CommandoAnatomy');me.from_pydata([cv(verts[i]) for i in used],[],[[mapping[i] for i in f] for f in faces]);me.update();body=bpy.data.objects.new('Commando',me);bpy.context.collection.objects.link(body)
uv=me.uv_layers.new(name='UVMap')
for poly,fu in zip(me.polygons,fuv):
 for li,ui in zip(poly.loop_indices,fu):uv.data[li].uv=uvs[ui]
 for v in [poly]:v.use_smooth=True
materials={}
def mat(name,c,rough=.65,metal=0):
 m=bpy.data.materials.new(name);m.diffuse_color=(*c,1);m.use_nodes=True;p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=(*c,1);p.inputs['Roughness'].default_value=rough;p.inputs['Metallic'].default_value=metal;materials[name]=m;return m
skin=mat('Commando_Skin',(.32,.17,.09),.57);camo=mat('Commando_Camo',(.09,.13,.055),.85);leather=mat('Commando_Leather',(.018,.021,.015),.64);hair=mat('Commando_Hair',(.016,.010,.006),.72);red=mat('Commando_Red',(.25,.018,.012),.85);metal=mat('Commando_Metal',(.038,.047,.038),.34,.7);eye=mat('Commando_Eye',(.6,.54,.4),.18)
for m in materials.values():body.data.materials.append(m)
for poly in body.data.polygons:
 c=poly.center
 # Polygon centre is calculated explicitly after source mesh construction.
 c=sum((me.vertices[i].co for i in poly.vertices),Vector())/len(poly.vertices)
 if c.z<.23:poly.material_index=2
 elif c.z<1.02:poly.material_index=1
 elif c.z<1.48 and abs(c.x)<.22:poly.material_index=1
 else:poly.material_index=0
armdata=bpy.data.armatures.new('CommandoSkeleton');arm=bpy.data.objects.new('CommandoRig',armdata);bpy.context.collection.objects.link(arm);bpy.context.view_layer.objects.active=arm;arm.select_set(True);bpy.ops.object.mode_set(mode='EDIT')
def bone(n,h,t,parent=None):
 b=armdata.edit_bones.new(n);b.head=h;b.tail=t
 if parent:b.parent=armdata.edit_bones[parent]
 return b
bone('root',(0,0,0),(0,0,.16));bone('pelvis',joints['joint-pelvis'],joints['joint-spine-3'],'root');bone('spine',joints['joint-spine-3'],joints['joint-spine-1'],'pelvis');bone('chest',joints['joint-spine-1'],joints['joint-neck'],'spine');bone('neck',joints['joint-neck'],joints['joint-head'],'chest');bone('head',joints['joint-head'],joints['joint-head-2'],'neck')
for s in ['l','r']:
 bone('thigh_'+s,joints['joint-'+s+'-upper-leg'],joints['joint-'+s+'-knee'],'pelvis');bone('calf_'+s,joints['joint-'+s+'-knee'],joints['joint-'+s+'-ankle'],'thigh_'+s);bone('foot_'+s,joints['joint-'+s+'-ankle'],joints['joint-'+s+'-foot-1'],'calf_'+s)
 bone('upperarm_'+s,joints['joint-'+s+'-shoulder'],joints['joint-'+s+'-elbow'],'chest');bone('forearm_'+s,joints['joint-'+s+'-elbow'],joints['joint-'+s+'-hand'],'upperarm_'+s);bone('hand_'+s,joints['joint-'+s+'-hand'],joints['joint-'+s+'-hand-3'],'forearm_'+s)
bpy.ops.object.mode_set(mode='OBJECT');bpy.ops.object.select_all(action='DESELECT');body.select_set(True);arm.select_set(True);bpy.context.view_layer.objects.active=arm
bpy.ops.object.parent_set(type='ARMATURE_AUTO')
print('AUTO_WEIGHT_GROUPS',len(body.vertex_groups),flush=True)
access=[]
def bind(o,name):
 vg=o.vertex_groups.new(name=name);vg.add(list(range(len(o.data.vertices))),1,'REPLACE');access.append(o);return o
def box(n,p,size,m,b='chest',bevel=.015):
 bpy.ops.mesh.primitive_cube_add(size=1,location=p);o=bpy.context.object;o.name=n;o.scale=size;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);o.data.materials.append(m)
 if bevel:
  mod=o.modifiers.new('Rounded stitched edges','BEVEL');mod.width=bevel;mod.segments=3;bpy.context.view_layer.objects.active=o;bpy.ops.object.modifier_apply(modifier=mod.name)
 for f in o.data.polygons:f.use_smooth=True
 return bind(o,b)
def sphere(n,p,size,m,b='head'):
 bpy.ops.mesh.primitive_uv_sphere_add(segments=24,ring_count=16,location=p);o=bpy.context.object;o.name=n;o.scale=size;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);o.data.materials.append(m)
 for f in o.data.polygons:f.use_smooth=True
 return bind(o,b)
def tube(n,points,r,m,b='head'):
 c=bpy.data.curves.new(n,'CURVE');c.dimensions='3D';c.bevel_depth=r;c.bevel_resolution=2;s=c.splines.new('POLY');s.points.add(len(points)-1)
 for v,p in zip(s.points,points):v.co=(*p,1)
 ob=bpy.data.objects.new(n,c);bpy.context.collection.objects.link(ob);ob.data.materials.append(m);bpy.ops.object.select_all(action='DESELECT');ob.select_set(True);bpy.context.view_layer.objects.active=ob;bpy.ops.object.convert(target='MESH');return bind(bpy.context.object,b)
# A fitted scalp shell follows the anatomical head rather than floating hair primitives.
scalpf=[]
for poly in body.data.polygons:
 c=sum((me.vertices[i].co for i in poly.vertices),Vector())/len(poly.vertices)
 if c.z>1.785 or (c.z>1.665 and c.y>-.045):scalpf.append(list(poly.vertices))
sv=sorted({i for f in scalpf for i in f});sm={v:i for i,v in enumerate(sv)}
scalp=bpy.data.meshes.new('HairScalp');center=Vector((0,-.043,1.76));scalp.from_pydata([center+(me.vertices[i].co-center)*1.035 for i in sv],[],[[sm[i] for i in f] for f in scalpf]);scalp.update();so=bpy.data.objects.new('HairScalp',scalp);bpy.context.collection.objects.link(so);so.data.materials.append(hair)
for f in so.data.polygons:f.use_smooth=True
bind(so,'head')
# Eye geometry, brows and an asymmetrical layered haircut.
for s in ['l','r']:
 p=joints['joint-'+s+'-eye'];sphere('Eye_'+s,p,(.013,.010,.010),eye);sphere('Iris_'+s,p+Vector((0,-.009,0)),(.005,.003,.006),hair)
head=joints['joint-head']; top=joints['joint-head-2']
for i in range(85):
 a=random.uniform(0,math.tau);h=random.uniform(-.03,.13);rr=.082*math.sqrt(max(.12,1-(h/.17)**2));p=Vector((math.cos(a)*rr,head.y+math.sin(a)*rr,head.z+h));q=p+Vector((random.uniform(-.02,.02),.025,-random.uniform(.05,.14)))
 if p.y<head.y-.04 and h<.06:continue
 tube('Hair strand',[p,(p+q)*.5+Vector((0,.02,.01)),q],random.uniform(.004,.008),hair)
# Cloth band follows the skull and has two loose tails.
pts=[(math.cos(i*math.tau/48)*.086,head.y+math.sin(i*math.tau/48)*.09,head.z+.075) for i in range(49)]
tube('Red headband',pts,.012,red);tube('Band tail L',[(.02,head.y+.09,head.z+.05),(.05,head.y+.17,head.z-.06),(.025,head.y+.18,head.z-.20)],.014,red);tube('Band tail R',[(-.01,head.y+.09,head.z+.05),(-.025,head.y+.18,head.z-.08),(-.075,head.y+.18,head.z-.18)],.01,red)
for side in [-1,1]:
 box('Shoulder harness',(side*.13,-.075,1.43),(.065,.055,.25),camo)
 box('Thigh cargo pocket',(side*.19,-.007,.80),(.12,.13,.18),camo,'thigh_'+('l' if side>0 else 'r'))
 box('Cargo flap',(side*.19,-.08,.845),(.123,.018,.055),camo,'thigh_'+('l' if side>0 else 'r'),.005)
 box('Boot sole',(side*.215,-.080,.028),(.135,.33,.052),leather,'foot_'+('l' if side>0 else 'r'),.009)
 box('Combat boot upper',(side*.21,-.075,.092),(.127,.29,.13),leather,'foot_'+('l' if side>0 else 'r'),.035)
 for z in [.13,.16,.19]:tube('Boot lace',[(side*.215-.04,-.083,z),(side*.215+.04,-.083,z)],.002,camo,'foot_'+('l' if side>0 else 'r'))
 for x in [.065,.13]:
  box('Magazine pouch',(side*x,-.135,1.15),(.061,.055,.14),camo,'spine',.008)
  box('Pouch buckle',(side*x,-.169,1.18),(.025,.008,.019),metal,'spine',.002)
box('Field pack',(0,.15,1.29),(.27,.16,.33),camo,'spine',.03)
box('Belt',(0,0,1.01),(.37,.19,.048),leather,'pelvis',.012);box('Belt clasp',(0,-.10,1.01),(.055,.01,.041),metal,'pelvis',.003)
# Merge accessory geometry into the skinned mesh while retaining authored weights.
bpy.ops.object.select_all(action='DESELECT');body.select_set(True)
for o in access:o.select_set(True)
bpy.context.view_layer.objects.active=body;bpy.ops.object.join();body.name='Commando'
# Inflate trousers slightly to read as cloth over the legs.
for v in body.data.vertices:
 if .24<v.co.z<.98:
  centre=.15 if v.co.x>0 else -.15;v.co.x=centre+(v.co.x-centre)*1.14;v.co.y*=1.12
# Static two-bone arm aiming constraints are baked into each animation.
for s,sign in [('l',1),('r',-1)]:
 target=bpy.data.objects.new('Aim_'+s,None);bpy.context.collection.objects.link(target);target.location=(sign*.12,-.42 if s=='l' else -.28,1.26)
 ik=arm.pose.bones['forearm_'+s].constraints.new('IK');ik.target=target;ik.chain_count=2
for pb in arm.pose.bones:pb.rotation_mode='XYZ'
bpy.context.scene.render.fps=30
for name,amplitude,frames in [('Idle',.025,90),('Walk',.42,30),('Run',.65,22)]:
 action=bpy.data.actions.new(name);arm.animation_data_create();arm.animation_data.action=action
 for frame in range(1,frames+2):
  t=(frame-1)/frames*math.tau
  for pb in arm.pose.bones:pb.rotation_euler=(0,0,0);pb.location=(0,0,0)
  arm.pose.bones['pelvis'].location.z=abs(math.sin(t))*amplitude*.045
  arm.pose.bones['spine'].rotation_euler.y=math.sin(t)*amplitude*.08
  for s,sgn in [('l',1),('r',-1)]:
   st=math.sin(t)*sgn;arm.pose.bones['thigh_'+s].rotation_euler.x=st*amplitude;arm.pose.bones['calf_'+s].rotation_euler.x=max(0,-st)*amplitude*1.3;arm.pose.bones['foot_'+s].rotation_euler.x=-st*amplitude*.25
  for pb in arm.pose.bones:pb.keyframe_insert('rotation_euler',frame=frame);pb.keyframe_insert('location',frame=frame)
 action.use_fake_user=True
bpy.context.scene.frame_start=1;bpy.context.scene.frame_end=90;arm.animation_data.action=bpy.data.actions['Idle'];bpy.context.scene.frame_set(1)
bpy.ops.object.select_all(action='DESELECT');body.select_set(True);arm.select_set(True);bpy.context.view_layer.objects.active=arm
bpy.ops.export_scene.fbx(filepath=str(out/'Commando.fbx'),use_selection=True,object_types={'MESH','ARMATURE'},add_leaf_bones=False,axis_forward='-Y',axis_up='Z',bake_anim=True,bake_anim_use_all_actions=True,bake_anim_use_nla_strips=False,bake_anim_simplify_factor=0)
bpy.ops.wm.save_as_mainfile(filepath=str(root/'Commando.blend'))
(out/'character_materials.json').write_text(json.dumps({n:{'base':list(m.diffuse_color),'roughness':m.node_tree.nodes.get('Principled BSDF').inputs['Roughness'].default_value,'metallic':m.node_tree.nodes.get('Principled BSDF').inputs['Metallic'].default_value} for n,m in materials.items()},indent=2))
print('COMMANDO_READY',len(body.data.vertices),'vertices',len(arm.data.bones),'bones',flush=True)
