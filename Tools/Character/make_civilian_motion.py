import bpy,sys,math,json
from pathlib import Path
from mathutils import Vector,Matrix,Quaternion
root=Path(__file__).resolve().parents[2]/'ArtSource/Production'
sys.path.insert(0,str(Path(__file__).resolve().parent))
from cmu_motion import read_asf,read_amc,fk
bpy.ops.wm.open_mainfile(filepath=str(root/'CommandoRigged.blend'))
arm=bpy.data.objects['CommandoRiggedRig'];body=bpy.data.objects['CommandoRigged'];scene=bpy.context.scene
motions={}
for key,subject in [('Walk','07'),('Run','09')]:
 bones,parents=read_asf(root/'Mocap'/(subject+'.asf'))
 motions[key]=[fk(bones,parents,f)[0] for f in read_amc(root/'Mocap'/(subject+'_01.amc'))]
def orient(name,direction):
 pb=arm.pose.bones[name];base=pb.bone.matrix_local.copy()
 head=(pb.parent.matrix@pb.parent.bone.matrix_local.inverted()@base).translation
 q=(base.to_3x3()@Vector((0,1,0))).rotation_difference(direction.normalized())
 pb.matrix=Matrix.Translation(head)@(q.to_matrix()@base.to_3x3()).to_4x4()
 bpy.context.view_layer.update()
results=[]
for name,base,count,kind,first,last in [('CivilianIdle','Idle',90,None,0,0),('CivilianWalk','WalkForward',33,'Walk',60,192),('CivilianRun','RunForward',22,'Run',0,88)]:
 action=bpy.data.actions[base].copy();action.name=name;action.use_fake_user=True;arm.animation_data.action=action
 arm.animation_data.action_slot=action.slots[0]
 changed=[]
 for side in ['l','r']:
  changed+=['clavicle_'+side,'upperarm_'+side,'forearm_'+side,'hand_'+side]
  changed+=['finger%d_%d_%s'%(finger,seg,side) for finger in range(1,6) for seg in range(1,4)]
 start_pose={}
 for fr in range(count+1):
  scene.frame_set(fr+1);phase=fr/count
  if kind:
   at=first+phase*(last-first);i=min(int(at),last);j=min(i+1,last)
   p={n:motions[kind][i][n].lerp(motions[kind][j][n],at-i) for n in motions[kind][i]}
  for side in ['l','r']:
   arm.pose.bones['clavicle_'+side].matrix_basis=Matrix.Identity(4)
   bpy.context.view_layer.update()
   if kind:
    orient('upperarm_'+side,p[side+'humerus']-p[side+'clavicle'])
    orient('forearm_'+side,p[side+'radius']-p[side+'humerus'])
   else:
    sign=1 if side=='l' else -1
    orient('upperarm_'+side,Vector((sign*.10,.04,-1)))
    orient('forearm_'+side,Vector((sign*.04,-.16,-1)))
   arm.pose.bones['hand_'+side].matrix_basis=Matrix.Identity(4)
   for finger in range(1,6):
    for seg in range(1,4):
     pb=arm.pose.bones['finger%d_%d_%s'%(finger,seg,side)]
     pb.rotation_quaternion=Quaternion((1,0,0),.13 if finger==1 else .19)
  bpy.context.view_layer.update()
  for n in changed:
   pb=arm.pose.bones[n]
   if fr==0:start_pose[n]=(pb.location.copy(),pb.rotation_quaternion.copy())
   if kind and fr>=count-4:
    weight=(fr-(count-4))/4
    pb.location=pb.location.lerp(start_pose[n][0],weight)
    pb.rotation_quaternion=pb.rotation_quaternion.slerp(start_pose[n][1],weight)
   pb.keyframe_insert('rotation_quaternion',frame=fr+1);pb.keyframe_insert('location',frame=fr+1)
  bpy.context.view_layer.update()
 scene.frame_start=1;scene.frame_end=count+1
 bpy.ops.object.select_all(action='DESELECT');arm.select_set(True);body.select_set(True);bpy.context.view_layer.objects.active=arm
 target=root/'Export'/(name+'.fbx')
 bpy.ops.export_scene.fbx(filepath=str(target),use_selection=True,object_types={'MESH','ARMATURE'},add_leaf_bones=False,axis_forward='-Y',axis_up='Z',bake_anim=True,bake_anim_use_all_actions=False,bake_anim_use_nla_strips=False,bake_anim_simplify_factor=0)
 results.append({'name':name,'base':base,'frames':count+1,'fps':30,'source':'CMU upper/lower body retarget' if kind else 'authored relaxed idle'})
 print('CIVILIAN_MOTION_READY',name,flush=True)
arm.animation_data.action=bpy.data.actions['CivilianWalk'];arm.animation_data.action_slot=arm.animation_data.action.slots[0]
scene.frame_start=1;scene.frame_end=34;scene.frame_set(17)
bpy.ops.wm.save_as_mainfile(filepath=str(root/'CivilianMotion.blend'))
(root/'Mocap/civilian_animation_manifest.json').write_text(json.dumps(results,indent=2))
# A quick shaded source review, kept separate from runtime proof.
world=bpy.data.worlds.new('Civilian Review World');scene.world=world;world.use_nodes=True;world.node_tree.nodes['Background'].inputs['Color'].default_value=(.08,.09,.08,1);world.node_tree.nodes['Background'].inputs['Strength'].default_value=.7
for name,pos,power,size in [('Key',(2,-3,4),500,4),('Fill',(-3,-1,2.5),300,3),('Rim',(0,3,3),450,2)]:
 data=bpy.data.lights.new(name,'AREA');data.energy=power;data.shape='DISK';data.size=size;ob=bpy.data.objects.new(name,data);scene.collection.objects.link(ob);ob.location=pos;ob.rotation_euler=(Vector((0,0,1))-ob.location).to_track_quat('-Z','Y').to_euler()
cam=bpy.data.objects.new('Review Camera',bpy.data.cameras.new('Review Camera'));scene.collection.objects.link(cam);cam.location=(3,-5,2.1);cam.rotation_euler=(Vector((0,0,.95))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.lens=65;scene.camera=cam
scene.render.engine='CYCLES';scene.cycles.samples=32;scene.render.resolution_x=700;scene.render.resolution_y=1000;scene.render.resolution_percentage=100;scene.render.filepath=str(root/'CivilianMotion-review.png')
bpy.ops.render.render(write_still=True)
