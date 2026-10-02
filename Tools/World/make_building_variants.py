import bpy,math,json
from pathlib import Path
exec((Path(__file__).resolve().parent/'geometry_helpers.py').read_text())
clear();w=mat('Field_Timber',(.17,.095,.04),.88);thatch=mat('Thatch_Reed',(.26,.23,.10),.98);iron=mat('Weathered_Iron',(.12,.15,.13),.64,.55)
# Raised riverside dwelling: open veranda, split bamboo walls, high pitched thatch roof.
for x in [-2.6,0,2.6]:
 for y in [-2.2,2.2]:pole('stilt',(x,y,-.1),(x,y,4.3),.10,w)
for i in range(27):box('raised floor',(-2.6+i*.2,0,1.13),(.185,5.9,.09),w)
for x in [-2.6,2.6]:
 for i in range(44):pole('bamboo wall',(x,-2.2+i*.1,1.2),(x,-2.2+i*.1,3.9),.042,w)
for i in range(53):
 x=-2.6+i*.1;pole('rear bamboo',(x,2.2,1.2),(x,2.2,3.9),.042,w)
 if abs(x)>.65:pole('front bamboo',(x,-2.2,1.2),(x,-2.2,3.9),.042,w)
for side in [-1,1]:
 for i in range(42):box('reed roofing',(-3.0+i*.145,side*1.45,4.26),(.17,3.35,.14),thatch,rot=(side*-.42,0,0),bev=.03)
for x in [-2.6,2.6]:pole('veranda rail',(x,-2.9,2.1),(x,-2.9,1.2),.06,w)
for i in range(6):box('entry stair',(0,-3.0-i*.28,1.0-i*.17),(1.4,.33,.16),w)
save('StiltHouse')
clear();brick=mat('Village_Plaster',(.43,.32,.20),.98);tile=mat('Roof_Terracotta',(.24,.075,.028),.94);wood=mat('Field_Timber',(.17,.095,.04),.88);blue=mat('Shutter_Blue',(.065,.16,.19),.84)
for x in [-3,3]:box('masonry side',(x,0,1.65),(.27,4.4,3.3),brick,bev=.04)
box('masonry rear',(0,2.2,1.65),(6,.27,3.3),brick)
for x in [-2.1,2.1]:
 box('front lower',(x,-2.2,.62),(1.75,.27,1.24),brick);box('front upper',(x,-2.2,2.83),(1.75,.27,.96),brick)
 for side in [-1,1]:box('open shutter',(x+side*.67,-2.45,1.79),(.5,.055,1.04),blue,rot=(0,0,side*.65))
box('door lintel',(0,-2.2,2.9),(2.5,.28,.82),brick)
for x in [-1.03,1.03]:box('door jamb',(x,-2.26,1.25),(.13,.14,2.5),wood)
for side in [-1,1]:
 box('roof base',(0,side*1.22,3.71),(6.7,2.78,.10),tile,rot=(side*-.34,0,0))
 for i in range(29):
  x=-3.25+i*.23
  pole('rounded roof tiles',(x,0,4.15),(x,side*2.6,3.23),.072,tile)
for x in [-2.9,2.9]:pole('awning post',(x,-3.7,0),(x,-3.7,2.48),.055,wood)
box('shop awning',(0,-2.96,2.65),(6.3,1.85,.08),blue,rot=(.19,0,0))
save('VillageStore')
clear();w=mat('Field_Timber',(.17,.095,.04),.88);sheet=mat('Shed_RustedRoof',(.23,.14,.08),.86,.55)
for x in [-2.2,2.2]:
 for y in [-1.8,1.8]:box('shed support',(x,y,1.5),(.13,.13,3),w)
for i in range(19):
 x=-2.2+i*.245;box('rear plank',(x,1.8,1.45),(.23,.08,2.8),w)
for x in [-2.2,2.2]:
 for i in range(15):box('side plank',(x,-1.7+i*.24,1.4),(.08,.22,2.8),w)
for i in range(31):box('corrugated roof',(-2.55+i*.17,0,3.18),(.19,4.5,.06),sheet,rot=(.17,0,0),bev=.008)
for x in [-1.5,0,1.5]:box('shed shelf',(x,.7,.9),(1.3,1.1,.08),w)
save('SupplyShed')
(out/'building_variants_manifest.json').write_text(json.dumps(manifest,indent=2));print('BUILDING_VARIANTS_READY',list(manifest),flush=True)
