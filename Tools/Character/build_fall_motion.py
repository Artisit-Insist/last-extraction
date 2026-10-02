"""Author weapon-release falls on the existing rig, preserving all earlier clips."""
import bpy,math,json
from pathlib import Path
from mathutils import Vector,Matrix,Quaternion
root=Path(__file__).resolve().parents[2]/'ArtSource/Production';out=root/'Animation07';out.mkdir(exist_ok=True)
bpy.ops.wm.open_mainfile(filepath=str(root/'Characters06/CharacterRoles06.blend'))
arm=bpy.data.objects['CommandoRiggedRig'];body=bpy.data.objects['Guard06'];scene=bpy.context.scene;scene.render.fps=30
body.hide_set(False);body.hide_render=False
arm.animation_data.action=bpy.data.actions['Idle'];arm.animation_data.action_slot=arm.animation_data.action.slots[0];scene.frame_set(1);bpy.context.view_layer.update()
idle={b.name:b.matrix_basis.copy() for b in arm.pose.bones};rest={b.name:b.matrix_local.copy() for b in arm.data.bones}
def ease(a,b,t):
 x=max(0,min(1,(t-a)/(b-a)));return x*x*(3-2*x)
def orient(name,direction,R,weight):
 pb=arm.pose.bones[name];m=pb.matrix.copy();basis=rest[name].to_3x3();d=Vector(direction).normalized()
 q=(basis@Vector((0,1,0))).rotation_difference(d)
 target=(R.to_3x3()@q.to_matrix()@basis).to_quaternion()
 rotation=m.to_quaternion().slerp(target,weight);pb.matrix=Matrix.Translation(m.translation)@rotation.to_matrix().to_4x4();bpy.context.view_layer.update()
def bend(name,axis,angle,R):
 pb=arm.pose.bones[name];m=pb.matrix.copy();q=Quaternion((R.to_3x3()@Vector(axis)).normalized(),angle)
 pb.matrix=Matrix.Translation(m.translation)@(q.to_matrix()@m.to_3x3()).to_4x4();bpy.context.view_layer.update()
spec={
 'FallBack07':{'axis':'X','angle':-.5*math.pi,'shift':(0,.12,0),'upper':[(.32,.025,-.94),(-.24,.035,-.97)],'fore':[(.48,.02,-.87),(-.24,.04,-.97)],'thigh':[(.14,-.42,-.9),(-.08,-.08,-.99)],'calf':[(0,.26,-.96),(0,.07,-.99)]},
 'FallSide07':{'axis':'Y','angle':-.5*math.pi,'shift':(-.15,0,0),'upper':[(.20,-.12,-.97),(-.02,-.1,-.99)],'fore':[(.1,-.45,-.9),(.10,-.5,-.87)],'thigh':[(-.24,-.58,-.78),(-.08,-.5,-.86)],'calf':[(-.2,.60,-.77),(0,.46,-.89)]},
 'FallForward07':{'axis':'X','angle':.5*math.pi,'shift':(0,-.15,0),'upper':[(.68,.02,-.73),(-.69,.03,-.72)],'fore':[(.08,.02,.99),(-.06,.04,.99)],'thigh':[(.13,.08,-.99),(-.16,.16,-.97)],'calf':[(0,-.06,-.99),(0,-.16,-.99)]}
}
left=dict(spec['FallSide07']);left['angle']=.5*math.pi;left['shift']=(.15,0,0)
for key in ['upper','fore','thigh','calf']:left[key]=[(-v[0],v[1],v[2]) for v in reversed(left[key])]
spec['FallLeft07']=left
report={}
for name,profile in spec.items():
 action=bpy.data.actions.new(name);action.use_fake_user=True;arm.animation_data.action=action
 frames=[];count=54
 for frame in range(count+1):
  scene.frame_set(frame+1)
  for pb in arm.pose.bones:pb.matrix_basis=idle[pb.name]
  t=frame/count;fall=ease(.12,.73,t);release=ease(.065,.48,t)
  R=Matrix.Rotation(profile['angle']*fall,4,profile['axis']);R.translation=Vector(profile['shift'])*fall
  arm.pose.bones['root'].matrix=R@rest['root'];bpy.context.view_layer.update()
  buckle=math.sin(math.pi*min(1,t/.65))*.38
  for i,side in enumerate(['l','r']):
   thigh=Vector(profile['thigh'][i]);thigh.y-=buckle;calf=Vector(profile['calf'][i]);calf.y+=buckle*1.7
   orient('thigh_'+side,thigh,R,ease(.03,.55,t));orient('calf_'+side,calf,R,ease(.03,.57,t))
   orient('upperarm_'+side,profile['upper'][i],R,release);orient('forearm_'+side,profile['fore'][i],R,release)
   orient('hand_'+side,profile['fore'][i],R,release)
   for finger in range(1,6):
    for segment in range(1,4):
     pb=arm.pose.bones[f'finger{finger}_{segment}_{side}'];pb.rotation_quaternion=idle[pb.name].to_quaternion().slerp(Quaternion((1,0,0),.14 if finger==1 else .10),ease(.01,.14,t))
  impact=math.sin(math.pi*min(1,t/.28))*.14
  settle=math.sin((t-.73)*math.pi*7)*math.exp(-(t-.73)*14)*.04 if t>.73 else 0
  bend('spine',(1,0,0),(-impact if profile['axis']=='X' else .06*fall)+settle,R)
  bend('head',(0,0,1),(.13 if name!='FallSide07' else -.2)*fall,R)
  bpy.context.view_layer.update()
  dg=bpy.context.evaluated_depsgraph_get();ev=body.evaluated_get(dg);mesh=ev.to_mesh();sole=min(v.co.z for v in mesh.vertices);ev.to_mesh_clear()
  m=arm.pose.bones['root'].matrix.copy();m.translation.z+=.006-sole;arm.pose.bones['root'].matrix=m;bpy.context.view_layer.update()
  for pb in arm.pose.bones:
   pb.keyframe_insert('rotation_quaternion',frame=frame+1);pb.keyframe_insert('location',frame=frame+1)
  frames.append({'frame':frame+1,'hand_l':list(arm.pose.bones['hand_l'].head),'hand_r':list(arm.pose.bones['hand_r'].head),'head':list(arm.pose.bones['head'].head),'root':list(arm.pose.bones['root'].head)})
 scene.frame_start=1;scene.frame_end=count+1
 bpy.ops.object.select_all(action='DESELECT');arm.select_set(True);body.select_set(True);bpy.context.view_layer.objects.active=arm
 bpy.ops.export_scene.fbx(filepath=str(out/(name+'.fbx')),use_selection=True,object_types={'MESH','ARMATURE'},add_leaf_bones=False,axis_forward='-Y',axis_up='Z',bake_anim=True,bake_anim_use_all_actions=False,bake_anim_use_nla_strips=False,bake_anim_simplify_factor=0)
 report[name]={'duration':count/30,'frames':frames,'release_seconds':.22,'source':'authored on existing 52 bone rig'};print('FALL07_READY',name,flush=True)
# Thicker clothing needs its own contact height while preserving the same motion.
for role,base,name in [('Heavy06','FallBack07','FallBackHeavy07'),('Scout06','FallForward07','FallForwardScout07'),('Worker06','FallBack07','FallBackWorker07')]:
 model=bpy.data.objects[role];model.hide_set(False);action=bpy.data.actions[base].copy();action.name=name;action.use_fake_user=True;arm.animation_data.action=action;arm.animation_data.action_slot=action.slots[0];corrections=[]
 for frame in range(1,56):
  scene.frame_set(frame);bpy.context.view_layer.update();ev=model.evaluated_get(bpy.context.evaluated_depsgraph_get());mesh=ev.to_mesh();sole=min(v.co.z for v in mesh.vertices);ev.to_mesh_clear()
  correction=max(0,.006-sole);pb=arm.pose.bones['root'];m=pb.matrix.copy();m.translation.z+=correction;pb.matrix=m;pb.keyframe_insert('location',frame=frame);corrections.append(correction)
 bpy.ops.object.select_all(action='DESELECT');arm.select_set(True);model.select_set(True);bpy.context.view_layer.objects.active=arm
 bpy.ops.export_scene.fbx(filepath=str(out/(name+'.fbx')),use_selection=True,object_types={'MESH','ARMATURE'},add_leaf_bones=False,axis_forward='-Y',axis_up='Z',bake_anim=True,bake_anim_use_all_actions=False,bake_anim_use_nla_strips=False,bake_anim_simplify_factor=0)
 report[name]={'duration':1.8,'base':base,'role':role,'ground_correction_meters':corrections,'release_seconds':.22,'source':'authored fall with clothing contact correction'};print('FALL07_READY',name,flush=True)
 model.hide_set(True)
arm.animation_data.action=bpy.data.actions['FallBack07'];arm.animation_data.action_slot=arm.animation_data.action.slots[0];scene.frame_set(55)
bpy.ops.wm.save_as_mainfile(filepath=str(out/'FallMotions07.blend'));(out/'fall_manifest.json').write_text(json.dumps(report,indent=2))
