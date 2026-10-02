import bpy,numpy as np,json
from pathlib import Path
r=Path(__file__).resolve().parents[2];p=r/'ArtSource/Production'
bpy.ops.wm.open_mainfile(filepath=str(p/'Animation07/FallMotions07.blend'))
a=bpy.data.objects['CommandoRiggedRig'];scene=bpy.context.scene;rows=[]
for role in ['Guard06','Scout06','Heavy06','Civilian06','Worker06']:
 ob=bpy.data.objects[role];ob.hide_set(False)
 for action in ['FallBack07','FallForward07','FallSide07','FallLeft07']:
  actual={('Heavy06','FallBack07'):'FallBackHeavy07',('Scout06','FallForward07'):'FallForwardScout07',('Worker06','FallBack07'):'FallBackWorker07'}.get((role,action),action)
  a.animation_data.action=bpy.data.actions[actual];a.animation_data.action_slot=a.animation_data.action.slots[0]
  for frame in range(1,56):
   scene.frame_set(frame);bpy.context.view_layer.update();ev=ob.evaluated_get(bpy.context.evaluated_depsgraph_get());mesh=ev.to_mesh();v=np.array([list(x.co) for x in mesh.vertices]);lo=v.min(axis=0);hi=v.max(axis=0);finite=bool(np.isfinite(v).all());ev.to_mesh_clear()
   row={'role':role,'action':action,'frame':frame,'finite':finite,'min':lo.tolist(),'max':hi.tolist()};rows.append(row)
   assert finite and max(hi-lo)<2.5 and lo[2]>-.004,row
result={'pose_count':len(rows),'all_passed':True,'checks':'finite vertices, extent under 2.5 m, min Z above -0.004 m; numerical checks do not certify naturalness','poses':rows}
(r/'Design/FALL_07_POSES.json').write_text(json.dumps(result,indent=2));print('FALL_POSE_CHECK_PASS',len(rows),flush=True)
