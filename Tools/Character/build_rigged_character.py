import sys,math,json
from pathlib import Path
from mathutils import Vector,Matrix,Quaternion
sys.path.insert(0,str(Path(__file__).resolve().parent))
from cmu_motion import read_asf,read_amc,fk
# Preserve the previous character. Build a new rig with clavicles, toe joints and fingers.
source=Path(__file__).with_name('create_commando.py').read_text().split('# Static two-bone arm aiming')[0]
source=source.replace("bone('upperarm_'+s,joints['joint-'+s+'-shoulder'],joints['joint-'+s+'-elbow'],'chest')", "bone('clavicle_'+s,joints['joint-'+s+'-clavicle'],joints['joint-'+s+'-shoulder'],'chest');bone('upperarm_'+s,joints['joint-'+s+'-shoulder'],joints['joint-'+s+'-elbow'],'clavicle_'+s)")
needle="bpy.ops.object.mode_set(mode='OBJECT');bpy.ops.object.select_all(action='DESELECT');body.select_set(True)"
extra="""for s in ['l','r']:
 bone('toe_'+s,joints['joint-'+s+'-foot-1'],joints['joint-'+s+'-toe-2-3'],'foot_'+s)
 for finger in range(1,6):
  for segment in range(1,4):
   name='finger%d_%d_%s'%(finger,segment,s)
   parent='hand_'+s if segment==1 else 'finger%d_%d_%s'%(finger,segment-1,s)
   bone(name,joints['joint-%s-finger-%d-%d'%(s,finger,segment)],joints['joint-%s-finger-%d-%d'%(s,finger,segment+1)],parent)
"""
source=source.replace(needle,extra+needle)
source='\n'.join(line for line in source.splitlines() if "box('Boot sole'" not in line and "box('Combat boot upper'" not in line)
exec(source)
arm.name='CommandoRiggedRig';body.name='CommandoRigged';arm.animation_data_create()
# Real image material stays in the editable source too.
t=camo.node_tree.nodes.new('ShaderNodeTexImage');t.image=bpy.data.images.load(str(root/'Camouflage.png'));camo.node_tree.links.new(t.outputs['Color'],camo.node_tree.nodes.get('Principled BSDF').inputs['Base Color'])
aim={}
for side,pos in [('r',(-.12,-.065,1.345)),('l',(.01,-.455,1.36))]:
 target=bpy.data.objects.new('Hand_IK_'+side,None);bpy.context.collection.objects.link(target);target.location=pos;aim[side]=target
 ik=arm.pose.bones['forearm_'+side].constraints.new('IK');ik.target=target;ik.chain_count=2
for pb in arm.pose.bones:pb.rotation_mode='QUATERNION'
rest={b.name:b.matrix_local.copy() for b in arm.data.bones};scene=bpy.context.scene;scene.render.fps=30
motions={}
for name,subject,start,end in [('Walk','07',60,192),('Run','09',0,88)]:
 b,pa=read_asf(root/'Mocap'/(subject+'.asf'));frames=read_amc(root/'Mocap'/(subject+'_01.amc'));poses=[fk(b,pa,f)[0] for f in frames]
 speed=(poses[end]['root']-poses[start]['root']).length/((end-start)/120)
 motions[name]=(poses,start,end,speed)
metadata={}
def reset():
 for pb in arm.pose.bones:pb.matrix_basis=Matrix.Identity(4)
 for side,pos in [('r',(-.12,-.065,1.345)),('l',(.01,-.455,1.36))]:aim[side].location=pos
 # Close the articulated fingers around the stock/grip instead of leaving flat hands.
 for side in ['l','r']:
  for finger in range(1,6):
   for segment in range(1,4):
    pb=arm.pose.bones['finger%d_%d_%s'%(finger,segment,side)];axis=Vector((1,0,0));angle=.40 if finger==1 else 1.05
    if side=='l':
     palm_axis=Vector((0,1,0)) if finger==1 else Vector((0,0,1))
     axis=pb.bone.matrix_local.to_3x3().inverted()@arm.data.bones['hand_l'].matrix_local.to_3x3()@palm_axis
     angle=([.8,.5,.4] if finger==1 else [.70,1.05,.75])[segment-1]
    pb.rotation_quaternion=Quaternion(axis.normalized(),angle)
def orient(name,direction,yaw=0):
 pb=arm.pose.bones[name];base=pb.bone.matrix_local.copy();head=(pb.parent.matrix @ pb.parent.bone.matrix_local.inverted() @ base).translation if pb.parent else base.translation
 d=Matrix.Rotation(yaw,3,'Z')@direction.normalized();q=(base.to_3x3()@Vector((0,1,0))).rotation_difference(d)
 pb.matrix=Matrix.Translation(head)@(q.to_matrix()@base.to_3x3()).to_4x4();bpy.context.view_layer.update()
def pose_walk(phase,kind,yaw=0,back=False):
 poses,first,last,speed=motions[kind];t=(1-phase if back else phase);at=first+t*(last-first);i=min(int(at),last);j=min(i+1,last);mix=at-i
 p={n:poses[i][n].lerp(poses[j][n],mix) for n in poses[i]}
 # Root translation stays in the game controller; retain the captured vertical weight shift.
 baseline=sum(poses[k]['root'].z for k in range(first,last+1))/(last-first+1)
 bob=(p['root'].z-baseline)*.8
 pel=arm.pose.bones['pelvis'];pel.matrix=Matrix.Translation(rest['pelvis'].translation+Vector((math.sin(phase*math.tau)*.012,0,bob)))@rest['pelvis'].to_3x3().to_4x4();bpy.context.view_layer.update()
 for side in ['l','r']:
  orient('thigh_'+side,p[side+'femur']-p[side+'hipjoint'],yaw)
  orient('calf_'+side,p[side+'tibia']-p[side+'femur'],yaw)
  orient('foot_'+side,p[side+'foot']-p[side+'tibia'],yaw*.35)
  orient('toe_'+side,p[side+'toes']-p[side+'foot'],yaw*.35)
 # Small torso counter-rotation and a stable head while holding the weapon.
 arm.pose.bones['spine'].rotation_quaternion=Quaternion((0,1,0),math.sin(phase*math.tau)*.025)
 arm.pose.bones['chest'].rotation_quaternion=Quaternion((0,1,0),-math.sin(phase*math.tau)*.018)
 return speed
spec=[('Idle',90,None,0,False),('WalkForward',33,'Walk',0,False),('WalkBack',33,'Walk',0,True),('WalkLeft',33,'Walk',-math.pi/2,False),('WalkRight',33,'Walk',math.pi/2,False),('RunForward',22,'Run',0,False),('RunBack',22,'Run',0,True),('RunLeft',22,'Run',-math.pi/2,False),('RunRight',22,'Run',math.pi/2,False),('Fire',8,None,0,False),('Reload',51,None,0,False),('Hit',15,None,0,False),('Dodge',21,None,0,False),('Death',45,None,0,False)]
for name,count,motion,yaw,back in spec:
 action=bpy.data.actions.new(name);action.use_fake_user=True;arm.animation_data.action=action;speed=0;poses_for_loop=[]
 # Bake constraints per-frame below, so the exported file needs no runtime Blender controls.
 for fr in range(count+1):
  scene.frame_set(fr+1);reset();bpy.context.view_layer.update();phase=fr/count
  if motion:speed=pose_walk(phase,motion,yaw,back)
  else:
   breathe=math.sin(phase*math.tau)*.008
   arm.pose.bones['chest'].location.y=breathe
   if name=='Idle':
    arm.pose.bones['spine'].rotation_quaternion=Quaternion((1,0,0),math.sin(phase*math.tau)*.009)
   if name=='Fire':
    recoil=math.sin(min(phase*3,1)*math.pi/2)*math.exp(-phase*5);arm.pose.bones['chest'].rotation_quaternion=Quaternion((1,0,0),-.055*recoil)
    for ob in aim.values():ob.location.y+=recoil*.035
   if name=='Reload':
    weight=math.sin(phase*math.pi)**1.4;aim['l'].location=Vector((.01,-.455,1.36)).lerp(Vector((.08,-.19,1.03)),weight);arm.pose.bones['head'].rotation_quaternion=Quaternion((1,0,0),.18*weight)
   if name=='Hit':
    w=math.sin(phase*math.pi)*math.exp(-phase*1.8);arm.pose.bones['chest'].rotation_quaternion=Quaternion((1,.25,0),-.27*w);arm.pose.bones['head'].rotation_quaternion=Quaternion((1,0,0),.14*w)
   if name=='Dodge':
    w=math.sin(phase*math.pi);pose_walk(phase,'Run');arm.pose.bones['spine'].rotation_quaternion=Quaternion((1,0,0),.30*w)
    for ob in aim.values():ob.location.z-=.08*w
   if name=='Death':
    t=phase*phase*(3-2*phase);R=Matrix.Rotation(-math.pi*.49*t,4,'X');R.translation=Vector((0,.35*t,.16*t));arm.pose.bones['root'].matrix=R@rest['root']
    for ob in aim.values():ob.location=R@ob.location
  bpy.context.view_layer.update()
  if motion or name in ['Dodge','Death']:
   dg=bpy.context.evaluated_depsgraph_get();evaluated=body.evaluated_get(dg)
   temp=evaluated.to_mesh();sole=min((body.matrix_world@v.co).z for v in temp.vertices);evaluated.to_mesh_clear()
   flight=0.0
   if motion=='Run':
    pp,ff,ll,_=motions[motion];ii=min(ll,int(ff+phase*(ll-ff)));floor=min(min(p['ltoes'].z,p['rtoes'].z,p['lfoot'].z,p['rfoot'].z) for p in pp[ff:ll+1]);flight=max(0,min(pp[ii]['ltoes'].z,pp[ii]['rtoes'].z,pp[ii]['lfoot'].z,pp[ii]['rfoot'].z)-floor)*.75
   shift=flight-sole+.006;rootmat=arm.pose.bones['root'].matrix.copy();rootmat.translation=rootmat.translation+Vector((0,0,shift));arm.pose.bones['root'].matrix=rootmat
   for ob in aim.values():ob.location.z+=shift
   bpy.context.view_layer.update()
  if motion and fr==0:print('GROUND_CHECK',name,'sole_before',sole,'shift',shift,'root',tuple(arm.pose.bones['root'].matrix.translation),flush=True)
  # Keep the support palm beneath the fore-end and the trigger palm around the grip.
  for side in ['l','r']:
   pb=arm.pose.bones['hand_'+side];location=pb.matrix.translation.copy()
   y=Vector((-1,0,0)) if side=='l' else Vector((0,-.35,-.937));x=Vector((0,0,-1)) if side=='l' else Vector((-1,0,0));z=x.cross(y).normalized()
   rotation=Matrix((x,y,z)).transposed().to_4x4()
   rotation=(R.to_3x3().to_4x4()@rotation) if name=='Death' else rotation;rotation.translation=location;pb.matrix=rotation;bpy.context.view_layer.update()
  # Store evaluated pose transforms and smooth the wrap seam in locomotion clips.
  matrices={pb.name:pb.matrix.copy() for pb in arm.pose.bones};poses_for_loop.append(matrices)
  for pb in arm.pose.bones:
   pb.keyframe_insert('rotation_quaternion',frame=fr+1);pb.keyframe_insert('location',frame=fr+1)
  for ob in aim.values():ob.keyframe_insert('location',frame=fr+1)
 # Bake IK arms into local transforms; child bones inherit the solved upper-arm pose.
 for pb in arm.pose.bones:
  for c in pb.constraints:c.mute=True
 for fr,matrices in enumerate(poses_for_loop):
  scene.frame_set(fr+1)
  desired_poses={}
  for pb in arm.pose.bones:
   desired=matrices[pb.name]
   if motion and fr>=count-4:
    weight=(fr-(count-4))/4;target=poses_for_loop[0][pb.name];q=desired.to_quaternion().slerp(target.to_quaternion(),weight);desired=Matrix.Translation(desired.translation.lerp(target.translation,weight))@q.to_matrix().to_4x4()
   desired_poses[pb.name]=desired
   if pb.parent:
    pb.matrix_basis=pb.bone.convert_local_to_pose(desired,pb.bone.matrix_local,parent_matrix=desired_poses[pb.parent.name],parent_matrix_local=pb.parent.bone.matrix_local,invert=True)
   else:pb.matrix_basis=pb.bone.convert_local_to_pose(desired,pb.bone.matrix_local,invert=True)
   pb.keyframe_insert('rotation_quaternion',frame=fr+1);pb.keyframe_insert('location',frame=fr+1)
 for pb in arm.pose.bones:
  for c in pb.constraints:c.mute=False
 metadata[name]={'duration_s':count/30,'source_speed_m_s':speed,'source':'CMU '+('07_01' if motion=='Walk' else '09_01') if motion else 'authored rig animation','loop':bool(motion) or name=='Idle'}
 print('BAKED',name,count+1,'frames',flush=True)
# Constraints are no longer needed by the baked actions.
for pb in arm.pose.bones:
 for c in list(pb.constraints):pb.constraints.remove(c)
for ob in aim.values():ob.animation_data_clear()
arm.animation_data.action=bpy.data.actions['Idle'];scene.frame_start=1;scene.frame_end=91;scene.frame_set(1)
bpy.ops.object.select_all(action='DESELECT');body.select_set(True);arm.select_set(True);bpy.context.view_layer.objects.active=arm
bpy.ops.export_scene.fbx(filepath=str(out/'CommandoRigged.fbx'),use_selection=True,object_types={'MESH','ARMATURE'},add_leaf_bones=False,axis_forward='-Y',axis_up='Z',bake_anim=True,bake_anim_use_all_actions=True,bake_anim_use_nla_strips=False,bake_anim_simplify_factor=0)
bpy.ops.wm.save_as_mainfile(filepath=str(root/'CommandoRigged.blend'))
(root/'Mocap/animation_manifest.json').write_text(json.dumps({'bones':len(arm.data.bones),'animations':metadata},indent=2))
print('RIGGED_CHARACTER_READY',len(arm.data.bones),flush=True)
