import bpy,math,json
from pathlib import Path
exec((Path(__file__).resolve().parent/'geometry_helpers.py').read_text())
clear();wood=mat('Field_Timber',(.14,.08,.034));steel=mat('Field_Steel',(.045,.052,.045),.35,.75)
# Origin is the hinge, so Unreal can swing the entire leaf without sliding it.
for i in range(23):
 y=.1+i*.2
 box('weathered plank',(0,y,1.31),(.075,.185,2.58),wood,bev=.013)
for z in [.28,2.27]:
 box('cross rail',(.085,2.31,z),(.09,4.72,.14),wood,bev=.015)
pole('diagonal brace',(.10,.16,.38),(.10,4.4,2.13),.068,wood)
for z in [.44,2.12]:
 cylinder('hinge barrel',(0,0,z),.058,.27,steel)
 box('hinge strap',(-.055,.4,z),(.024,.85,.10),steel,bev=.006)
 for y in [.14,.42,.70]:
  cylinder('rivet',(-.079,y,z),.018,.012,steel,rot=(0,math.pi/2,0))
box('latch plate',(-.05,4.23,1.22),(.023,.25,.32),steel,bev=.015)
pole('pull handle',(-.14,4.18,1.08),(-.14,4.18,1.36),.023,steel)
save('BridgeGateLeaf')
clear();wood=mat('Field_Timber',(.14,.08,.034));steel=mat('Field_Steel',(.045,.052,.045),.35,.75)
for y in [-.12,4.78]:
 box('gate post',(0,y,1.62),(.25,.25,3.24),wood,bev=.02)
 box('post shoe',(0,y,.18),(.32,.32,.36),steel,bev=.015)
 for z in [.52,2.55]:box('post band',(0,y,z),(.27,.27,.085),steel,bev=.006)
box('header beam',(0,2.33,3.12),(.23,5.15,.22),wood,bev=.02)
save('BridgeGateFrame')
(out/'gate_manifest.json').write_text(json.dumps(manifest,indent=2));print('GATE_MODELS_READY',flush=True)
