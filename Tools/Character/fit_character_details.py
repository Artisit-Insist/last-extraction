import bpy,bmesh,math,json
from pathlib import Path
from mathutils import Vector
root=Path(__file__).resolve().parents[2]/'ArtSource/Production'
bpy.ops.wm.open_mainfile(filepath=str(root/'CommandoRigged.blend'))
body=bpy.data.objects['CommandoRigged'];arm=bpy.data.objects['CommandoRiggedRig']
verts=[];body_ids=set();group=''
for l in (root/'human_base/base.obj').read_text().splitlines():
 s=l.split()
 if not s:continue
 if s[0]=='v':verts.append(Vector(map(float,s[1:4])))
 elif s[0]=='g':group=s[1]
 elif s[0]=='f' and group=='body':body_ids.update(int(v.split('/')[0])-1 for v in s[1:])
for name in ['caucasian-male-young.target','universal-male-young-maxmuscle-averageweight.target']:
 for l in (root/'human_base'/name).read_text().splitlines():
  if not l or l.startswith('#'):continue
  s=l.split();verts[int(s[0])]+=Vector(map(float,s[1:4]))
low=min(verts[i].y for i in body_ids);scale=1.86/(max(verts[i].y for i in body_ids)-low)
def cv(v):return Vector((v.x*scale,-v.z*scale,(v.y-low)*scale))

def fit(folder,name,material_name,rough):
 folder=root/'MakeHumanSystemAssets'/folder
 refs=[];scaling=Vector((1,1,1))
 for l in (folder/(name+'.mhclo')).read_text().splitlines():
  p=l.split()
  if not p:continue
  if p[0] in ['x_scale','y_scale','z_scale']:
   axis={'x_scale':0,'y_scale':1,'z_scale':2}[p[0]];scaling[axis]=abs(verts[int(p[1])][axis]-verts[int(p[2])][axis])/float(p[3])
  elif len(p)==9 and all(t.lstrip('-').isdigit() for t in p[:3]):refs.append(([int(n) for n in p[:3]],[float(n) for n in p[3:6]],Vector(map(float,p[6:9]))))
 points=[cv(sum((verts[i]*w for i,w in zip(ids,weights)),Vector())+Vector((offset.x*scaling.x,offset.y*scaling.y,offset.z*scaling.z))) for ids,weights,offset in refs]
 uv=[];faces=[];uvfaces=[];expected=0
 for l in (folder/(name+'.obj')).read_text().splitlines():
  p=l.split()
  if not p:continue
  if p[0]=='v':expected+=1
  if p[0]=='vt':uv.append(tuple(map(float,p[1:3])))
  if p[0]=='f':faces.append([int(x.split('/')[0])-1 for x in p[1:]]);uvfaces.append([int(x.split('/')[1])-1 for x in p[1:]])
 assert expected==len(points),(name,expected,len(points))
 mesh=bpy.data.meshes.new(name);mesh.from_pydata(points,[],faces);mesh.update();layer=mesh.uv_layers.new(name='UVMap')
 for f,uvs in zip(mesh.polygons,uvfaces):
  f.use_smooth=True
  for li,ui in zip(f.loop_indices,uvs):layer.data[li].uv=uv[ui]
 ob=bpy.data.objects.new(name,mesh);bpy.context.collection.objects.link(ob)
 mat=bpy.data.materials.new(material_name);mat.use_nodes=True;bs=mat.node_tree.nodes['Principled BSDF'];bs.inputs['Roughness'].default_value=rough
 tex=mat.node_tree.nodes.new('ShaderNodeTexImage');tex.image=bpy.data.images.load(str(folder/(name+'_diffuse.png')));mat.node_tree.links.new(tex.outputs['Color'],bs.inputs['Base Color'])
 if name=='short02':mat.node_tree.links.new(tex.outputs['Alpha'],bs.inputs['Alpha']);mat.surface_render_method='DITHERED'
 mesh.materials.append(mat);return ob

# Replace the fused anatomical toe shell with the CC0 fitted hiking boots.
bm=bmesh.new();bm.from_mesh(body.data);remove=[f for f in bm.faces if f.material_index==2 and max(v.co.z for v in f.verts)<.245];bmesh.ops.delete(bm,geom=remove,context='FACES');bm.to_mesh(body.data);bm.free()
boots=fit('clothes/shoes02','shoes02','Commando_Boots03',.8)
for side,sign in [('l',1),('r',-1)]:
 foot=boots.vertex_groups.new(name='foot_'+side);calf=boots.vertex_groups.new(name='calf_'+side)
 for v in boots.data.vertices:
  if v.co.x*sign<0:continue
  weight=max(0,min(1,(v.co.z-.15)/.09))
  if weight:calf.add([v.index],weight,'REPLACE')
  if weight<1:foot.add([v.index],1-weight,'REPLACE')
hair=fit('hair/short02','short02','Commando_Hair03',.84);hair.vertex_groups.new(name='head').add(list(range(len(hair.data.vertices))),1,'REPLACE')
bpy.ops.object.select_all(action='DESELECT');body.select_set(True);boots.select_set(True);hair.select_set(True);bpy.context.view_layer.objects.active=body;bpy.ops.object.join();body.name='CommandoVisual03'
skin=bpy.data.materials['Commando_Skin'];tex=skin.node_tree.nodes.new('ShaderNodeTexImage');tex.image=bpy.data.images.load(str(root/'MakeHumanSystemAssets/skins/young_caucasian_male/young_lightskinned_male_diffuse.png'));skin.node_tree.links.new(tex.outputs['Color'],skin.node_tree.nodes['Principled BSDF'].inputs['Base Color'])
arm.animation_data.action=bpy.data.actions['Idle'];arm.animation_data.action_slot=arm.animation_data.action.slots[0];bpy.context.scene.frame_set(1)
bpy.ops.object.select_all(action='DESELECT');body.select_set(True);arm.select_set(True);bpy.context.view_layer.objects.active=arm
bpy.ops.export_scene.fbx(filepath=str(root/'Export/CommandoVisual03.fbx'),use_selection=True,object_types={'MESH','ARMATURE'},add_leaf_bones=False,axis_forward='-Y',axis_up='Z',bake_anim=False)
bpy.ops.wm.save_as_mainfile(filepath=str(root/'CommandoVisual03.blend'));print('CHARACTER_VISUAL03_READY',len(body.data.vertices),len(arm.data.bones),flush=True)
