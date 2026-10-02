import bpy,json
from pathlib import Path
from mathutils import Vector,Matrix
root=Path(__file__).resolve().parents[2]/'ArtSource/Production'
bpy.ops.wm.open_mainfile(filepath=str(root/'CivilianMotion.blend'))
arm=bpy.data.objects['CommandoRiggedRig'];body=bpy.data.objects['CommandoRigged'];scene=bpy.context.scene
base=bpy.data.actions['CivilianIdle'];arm.animation_data.action=base;arm.animation_data.action_slot=base.slots[0];scene.frame_set(1)
bases={p.name:p.matrix_basis.copy() for p in arm.pose.bones};pelvis=arm.pose.bones['pelvis'].matrix.copy()
action=base.copy();action.name='CivilianSit';action.use_fake_user=True;arm.animation_data.action=action;arm.animation_data.action_slot=action.slots[0]
def orient(name,direction):
 p=arm.pose.bones[name];rest=p.bone.matrix_local.copy();head=(p.parent.matrix@p.parent.bone.matrix_local.inverted()@rest).translation
 q=(rest.to_3x3()@Vector((0,1,0))).rotation_difference(direction.normalized())
 p.matrix=Matrix.Translation(head)@(q.to_matrix()@rest.to_3x3()).to_4x4();bpy.context.view_layer.update()
for frame in range(1,32):
 scene.frame_set(frame)
 for p in arm.pose.bones:p.matrix_basis=bases[p.name]
 t=(frame-1)/30;w=t*t*(3-2*t);m=pelvis.copy();m.translation.z-=.54*w;arm.pose.bones['pelvis'].matrix=m;bpy.context.view_layer.update()
 for side in ['l','r']:
  name='thigh_'+side;rest=arm.data.bones[name].matrix_local.to_3x3()@Vector((0,1,0));orient(name,rest.lerp(Vector((0,-1,-.035)),w))
  name='calf_'+side;rest=arm.data.bones[name].matrix_local.to_3x3()@Vector((0,1,0));orient(name,rest.lerp(Vector((0,.03,-1)),w))
  for name in ['foot_'+side,'toe_'+side]:orient(name,arm.data.bones[name].matrix_local.to_3x3()@Vector((0,1,0)))
 for p in arm.pose.bones:
  p.keyframe_insert('location',frame=frame);p.keyframe_insert('rotation_quaternion',frame=frame);p.keyframe_insert('scale',frame=frame)
scene.frame_start=1;scene.frame_end=31;bpy.ops.object.select_all(action='DESELECT');arm.select_set(True);body.select_set(True);bpy.context.view_layer.objects.active=arm
bpy.ops.export_scene.fbx(filepath=str(root/'Export/CivilianSit.fbx'),use_selection=True,object_types={'MESH','ARMATURE'},add_leaf_bones=False,axis_forward='-Y',axis_up='Z',bake_anim=True,bake_anim_use_all_actions=False,bake_anim_use_nla_strips=False,bake_anim_simplify_factor=0)
bpy.ops.wm.save_as_mainfile(filepath=str(root/'CivilianBoarding.blend'));print('CIVILIAN_SIT_READY',flush=True)
