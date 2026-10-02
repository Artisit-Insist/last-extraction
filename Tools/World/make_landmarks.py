import bpy,math,json
from pathlib import Path
exec((Path(__file__).resolve().parent/'geometry_helpers.py').read_text())
clear();wood=mat('Field_Timber',(.14,.08,.034));steel=mat('Field_Steel',(.045,.052,.045),.35,.75);rope=mat('Bridge_Rope',(.13,.10,.055),.92)
# A 26 m suspended timber bridge with 4.6 m clear deck and visible anchor cables.
for i in range(130):
 x=-12.9+i*.2;z=.8-.035*math.sin(math.pi*i/129)
 box('deck plank',(x,0,z),(.188,4.6,.11),wood,bev=.012)
for x in [-12.6,12.6]:
 for y in [-2.55,2.55]:
  box('bridge tower',(x,y,1.65),(.28,.28,4.3),wood,bev=.025)
  box('anchor footing',(x,y,-.25),(.8,.8,.65),steel,bev=.07)
 for y in [-2.55,2.55]:
  pole('tower cross brace',(x-.3,y,-.4),(x+.3,y,3.5),.075,wood)
for y in [-2.5,2.5]:
 for i in range(52):
  x=-13+i*.5;xx=x+.5;z=1.95+1.4*(abs(x)/13)**2;zz=1.95+1.4*(abs(xx)/13)**2
  pole('suspension cable',(x,y,z),(xx,y,zz),.028,steel)
  if i%2==0:pole('vertical cable',(x,y,.82),(x,y,z),.013,steel)
  pole('hand rope',(x,y,1.65),(xx,y,1.65),.026,rope)
for x in [-14.5,14.5]:
 box('bank ramp',(x,0,.36),(3.4,4.6,.18),wood,rot=(0,(-1 if x<0 else 1)*.235,0))
save('RiverBridge')
clear();wood=mat('Field_Timber',(.14,.08,.034));steel=mat('Field_Steel',(.045,.052,.045),.35,.75);red=mat('Bridge_Red',(.23,.027,.015),.5)
box('winch timber stand',(0,0,.45),(1.35,.8,.9),wood)
for x in [-.4,.4]:box('winch side',(x,0,1.1),(.1,.58,.65),steel)
cylinder('cable drum',(0,0,1.13),.22,.76,steel,rot=(0,math.pi/2,0))
for i in range(16):
 x=-.35+i*.045
 for j in range(20):
  a=j*math.tau/20;b=(j+1)*math.tau/20;pole('wound cable',(x,math.cos(a)*.23,1.13+math.sin(a)*.23),(x,math.cos(b)*.23,1.13+math.sin(b)*.23),.01,steel)
pole('crank',(0.52,0,1.13),(.52,0,1.5),.025,steel);pole('red handle',(.52,0,1.5),(.72,0,1.5),.045,red)
box('switch plate',(0,-.42,.95),(.5,.025,.2),steel);box('switch',(0,-.46,.95),(.08,.06,.09),red)
save('BridgeWinch')
clear();steel=mat('Field_Steel',(.045,.052,.045),.35,.75);yellow=mat('Crane_Ochre',(.28,.19,.025),.64,.6)
for x in [-3,3]:
 for y in [-2,2]:pole('gantry leg',(x,y,0),(x,y,10),.12,steel)
for z in [1,4,7,10]:
 for y in [-2,2]:pole('horizontal frame',(-3,y,z),(3,y,z),.1,yellow)
 for x in [-3,3]:pole('depth frame',(x,-2,z),(x,2,z),.1,yellow)
for y in [-2,2]:
 for x in [-3,0]:
  for z in [1,4,7]:pole('triangular brace',(x,y,z),(x+3,y,z+3),.05,steel)
for y in [-.7,.7]:
 pole('boom lower',(-2,y,10),(11,y,10),.14,yellow);pole('boom upper',(-2,y,11.5),(11,y,10),.14,yellow)
 for i in range(12):pole('boom lattice',(-1+i,y,10),(-.5+i,y,11.35-i*.11),.06,steel)
box('counterweight',(-2.6,0,10.35),(2.1,2.2,1.4),steel);box('operator cab',(1,-2,5.3),(2.2,1.9,2.1),yellow)
for y in [-.4,.4]:pole('hoist cable',(9,y,10),(9,y,1.7),.025,steel)
box('hoist block',(9,0,1.6),(.7,1,.6),yellow)
save('QuarryCrane')
(out/'landmark_manifest.json').write_text(json.dumps(manifest,indent=2));print('LANDMARKS_READY',list(manifest),flush=True)
