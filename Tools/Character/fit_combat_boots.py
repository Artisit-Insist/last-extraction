import bpy,bmesh
from pathlib import Path
from mathutils import Vector
root=Path(__file__).resolve().parents[2]/'ArtSource/Production';bpy.ops.wm.open_mainfile(filepath=str(root/'CommandoRigged.blend'))
body=bpy.data.objects['CommandoRigged'];arm=bpy.data.objects['CommandoRiggedRig'];arm.animation_data.action=bpy.data.actions['Idle'];bpy.context.scene.frame_set(1)
for side,sign in [('l',1),('r',-1)]:
 polygons=[]
 for poly in body.data.polygons:
  vs=[body.data.vertices[i].co for i in poly.vertices]
  if poly.material_index==2 and max(v.z for v in vs)<.245 and all(v.x*sign>0 for v in vs):polygons.append(list(poly.vertices))
 used=sorted({i for f in polygons for i in f});indices={n:i for i,n in enumerate(used)};coords=[]
 for i in used:
  v=body.data.vertices[i].co.copy();v.x=sign*.205+(v.x-sign*.205)*1.14;v.y=-.03+(v.y+.03)*1.065;v.z=max(.006,v.z-.005);coords.append(v)
 me=bpy.data.meshes.new('Fitted leather boot');me.from_pydata(coords,[],[[indices[i] for i in f] for f in polygons]);me.update();boot=bpy.data.objects.new('CombatBoot_'+side,me);bpy.context.collection.objects.link(boot)
 bm=bmesh.new();bm.from_mesh(me);bmesh.ops.holes_fill(bm,edges=[e for e in bm.edges if e.is_boundary],sides=0);bmesh.ops.convex_hull(bm,input=list(bm.verts),use_existing_faces=False);bm.to_mesh(me);bm.free()
 bpy.ops.object.select_all(action='DESELECT');boot.select_set(True);bpy.context.view_layer.objects.active=boot
 mod=boot.modifiers.new('Fused leather toe cap','REMESH');mod.mode='VOXEL';mod.voxel_size=.006;mod.use_smooth_shade=True;bpy.ops.object.modifier_apply(modifier=mod.name)
 mod=boot.modifiers.new('Leather contours','SMOOTH');mod.factor=1.1;mod.iterations=4;bpy.ops.object.modifier_apply(modifier=mod.name)
 boot.data.materials.append(bpy.data.materials['Commando_Leather'])
 foot=boot.vertex_groups.new(name='foot_'+side);calf=boot.vertex_groups.new(name='calf_'+side);toe=boot.vertex_groups.new(name='toe_'+side)
 for v in boot.data.vertices:
  c=max(0,min(1,(v.co.z-.15)/.08));t=0
  if c:calf.add([v.index],c,'REPLACE')
  if t:toe.add([v.index],t,'REPLACE')
  if 1-c-t>0:foot.add([v.index],1-c-t,'REPLACE')
 body.select_set(True);bpy.context.view_layer.objects.active=body;bpy.ops.object.join()
 bpy.ops.object.select_all(action='DESELECT')
# Keep the underlying toes inside the rigid leather toe cap.
for v in body.data.vertices:
 if v.co.z<.13:
  for vg in body.vertex_groups:vg.remove([v.index])
  body.vertex_groups['foot_'+('l' if v.co.x>0 else 'r')].add([v.index],1,'REPLACE')
body.select_set(True);arm.select_set(True);bpy.context.view_layer.objects.active=arm
bpy.ops.export_scene.fbx(filepath=str(root/'Export/CommandoRigged.fbx'),use_selection=True,object_types={'MESH','ARMATURE'},add_leaf_bones=False,axis_forward='-Y',axis_up='Z',mesh_smooth_type='FACE',bake_anim=True,bake_anim_use_all_actions=True,bake_anim_use_nla_strips=False,bake_anim_simplify_factor=0)
bpy.ops.wm.save_as_mainfile(filepath=str(root/'CommandoRigged.blend'))
print('FITTED_BOOTS_READY')
