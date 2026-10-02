import bpy,math,random,json
from pathlib import Path
from mathutils import Vector
exec((Path(__file__).resolve().parent/'geometry_helpers.py').read_text())
def setup():
 clear()
 return mat('Field_Timber',(.18,.11,.05),.87),mat('Weathered_Iron',(.12,.15,.13),.64,.55),mat('Canvas_Tan',(.38,.28,.12),.98),mat('Field_Rubber',(.018,.019,.016),.92)
def ring(name,p,r,t,m,rot=(0,0,0)):
 bpy.ops.mesh.primitive_torus_add(major_radius=r,minor_radius=t,major_segments=32,minor_segments=8,location=p,rotation=rot);o=bpy.context.object;o.name=name;o.data.materials.append(m);return o
w,s,c,k=setup()
# Low wooden fishing canoe with a continuous curved hull and separate ribs.
verts=[];faces=[]
for i in range(25):
 x=-3+i*.25;width=.66*math.sin(math.pi*(i+1)/26)**.75
 for j in range(9):
  a=-math.pi/2+j*math.pi/8;verts.append((x,width*math.sin(a),.22+.55*(1-math.cos(a))+.3*(abs(x)/3)**3))
for i in range(24):
 for j in range(8):a=i*9+j;faces.append((a,a+1,a+10,a+9))
mesh=bpy.data.meshes.new('canoe hull');mesh.from_pydata(verts,[],faces);mesh.materials.append(w);o=bpy.data.objects.new('planked hull',mesh);bpy.context.collection.objects.link(o);sol=o.modifiers.new('hull thickness','SOLIDIFY');sol.thickness=.045;bpy.context.view_layer.objects.active=o;bpy.ops.object.modifier_apply(modifier=sol.name)
for x in [-1.5,-.3,1.2]:box('cross seat',(x,0,.65),(.28,1.12,.075),w)
pole('paddle',(-1,-.3,.74),(1.4,.22,.74),.027,w);box('paddle blade',(1.58,.26,.74),(.6,.22,.035),w,rot=(0,0,.2));save('FishingCanoe')
w,s,c,k=setup()
for i in range(28):box('dock plank',(i*.22,0,.8),(.205,2.3,.09),w)
for x in [.2,3,5.7]:
 for y in [-1,1]:pole('pier piling',(x,y,-1.4),(x,y,1.25),.10,w)
for y in [-.8,.8]:box('stringer',(3,y,.63),(6.4,.14,.22),w)
ring('mooring rope coil',(4.7,.5,.89),.25,.025,c);save('RiverDock')
w,s,c,k=setup()
for x in [-1.25,1.25]:pole('net pole',(x,0,0),(x,0,2.2),.065,w)
for i in range(18):
 x=-1.2+i*2.4/17;pole('net vertical',(x,0,.35+.2*math.cos(x)),(x,0,1.9-.35*(1-(x/1.2)**2)),.008,c)
for j in range(12):
 z=.5+j*.11;pole('net horizontal',(-1.2,0,z),(1.2,0,z),.008,c)
for x in [-.8,.3]:ring('fish basket rim',(x,.5,.55),.28,.025,w);cylinder('fish basket',(x,.5,.28),.27,.52,w)
save('FishingNetRack')
w,s,c,k=setup();stone=mat('Well_Stone',(.31,.30,.24),.95)
for row in range(4):
 for i in range(16):a=(i+(row%2)*.5)*math.tau/16;box('well masonry',(.95*math.cos(a),.95*math.sin(a),.16+row*.26),(.37,.29,.245),stone,rot=(0,0,a+math.pi/2),bev=.045)
for x in [-1.25,1.25]:box('well upright',(x,0,1.55),(.16,.16,3.1),w)
pole('well axle',(-1.4,0,2.25),(1.4,0,2.25),.09,w);pole('bucket rope',(0,0,2.25),(0,0,.35),.017,c)
for side in [-1,1]:box('well roof',(0,side*.65,3.08),(3.3,1.5,.12),w,rot=(side*-.35,0,0))
cylinder('bucket',(.2,0,.53),.25,.4,s);save('VillageWell')
w,s,c,k=setup();green=mat('Produce_Green',(.13,.26,.07));orange=mat('Produce_Ochre',(.58,.24,.035))
for x in [-1.65,1.65]:
 for y in [-.8,.8]:box('stall posts',(x,y,1.2),(.085,.085,2.4),w)
box('counter',(0,0,.88),(3.5,1.75,.13),w)
for i in range(12):box('striped canopy',((i-5.5)*.31,0,2.5),(.315,2.1,.045),c if i%2 else green,rot=(.10,0,0),bev=.01)
for x in [-1.1,0,1.1]:
 box('produce tray',(x,0,1.03),(.88,1.38,.18),w)
 for j in range(14):
  bpy.ops.mesh.primitive_uv_sphere_add(segments=12,ring_count=8,radius=.085,location=(x+random.uniform(-.30,.30),random.uniform(-.52,.52),1.16));bpy.context.object.data.materials.append(orange if x==0 else green)
save('MarketStall')
w,s,c,k=setup();blue=mat('Washed_Cloth_Blue',(.10,.19,.24),1);white=mat('Washed_Cloth_Cream',(.47,.43,.33),1)
for x in [-2.4,2.4]:pole('laundry post',(x,0,0),(x,0,2.4),.06,w)
pole('laundry rope',(-2.4,0,2.25),(2.4,0,2.25),.012,c)
for i in range(6):
 x=-1.9+i*.75;box('hanging cloth',(x,0,1.75),(.58,.028,.91),blue if i%2 else white,rot=(0,.035*(i-2),.015),bev=.018)
 for dx in [-.2,.2]:box('wooden peg',(x+dx,-.025,2.22),(.035,.045,.13),w)
save('Clothesline')
w,s,c,k=setup()
for x in [-.65,.65]:cylinder('reel flange',(x,0,.91),.91,.13,w,rot=(0,math.pi/2,0))
cylinder('reel core',(0,0,.91),.55,1.3,w,rot=(0,math.pi/2,0))
for i in range(28):ring('wound cable',(-.57+i*.043,0,.91),.60,.022,k,rot=(0,math.pi/2,0))
for x in [-.75,.75]:cylinder('axle end',(x,0,.91),.14,.13,s,rot=(0,math.pi/2,0))
save('CableReel')
w,s,c,k=setup();rust=mat('Mine_Rust',(.22,.095,.03),.88,.5)
box('cart floor',(0,0,.67),(2,1.4,.16),rust)
for y in [-.7,.7]:box('cart wall',(0,y,1.1),(2,.09,.95),rust,rot=(-.15*y,0,0))
for x in [-1,1]:box('cart end',(x,0,1.1),(.09,1.4,.95),rust,rot=(0,-.1*x,0))
for x in [-.65,.65]:
 pole('cart axle',(x,-.92,.36),(x,.92,.36),.065,s)
 for y in [-.9,.9]:cylinder('rail wheel',(x,y,.36),.33,.13,s,rot=(math.pi/2,0,0))
for y in [-.9,.9]:box('rail',(0,y,.06),(7,.075,.12),s)
for i in range(12):box('sleeper',(-3.3+i*.6,0,-.015),(.2,2.2,.13),w)
save('MineCart')
w,s,c,k=setup();yellow=mat('Equipment_Yellow',(.43,.29,.04),.6,.5)
box('compressor',(0,0,.87),(2.4,1.2,1.1),yellow,bev=.15)
for y in [-.69,.69]:cylinder('compressor tire',(.15,y,.42),.41,.23,k,rot=(math.pi/2,0,0))
for i in range(12):box('vent slat',(-.85+i*.13,-.609,1.05),(.07,.02,.48),s,bev=.005)
pole('tow arm',(1.2,0,.45),(2.3,0,.3),.055,s);ring('tow eye',(2.36,0,.3),.11,.025,s)
for i in range(5):ring('coiled air hose',(-1.45,0,.10+i*.055),.4,.025,k)
save('QuarryCompressor')
w,s,c,k=setup();olive=mat('Radar_Olive',(.1,.15,.08),.69,.35)
box('radar pedestal',(0,0,.5),(1.5,1.5,1),olive)
cylinder('radar mast',(0,0,2.3),.12,3.5,s)
verts=[(0,0,0)];faces=[]
for ringid in range(1,7):
 r=ringid*.24
 for i in range(32):a=i*math.tau/32;verts.append((r*math.cos(a),.23*r*r,2.9+r*math.sin(a)))
verts[0]=(0,0,2.9)
for i in range(32):faces.append((0,1+i,1+(i+1)%32))
for j in range(5):
 for i in range(32):a=1+j*32+i;b=1+j*32+(i+1)%32;faces.append((a,b,b+32,a+32))
mesh=bpy.data.meshes.new('radar dish');mesh.from_pydata(verts,[],faces);mesh.materials.append(olive);o=bpy.data.objects.new('parabolic dish',mesh);bpy.context.collection.objects.link(o);mod=o.modifiers.new('dish thickness','SOLIDIFY');mod.thickness=.035;bpy.context.view_layer.objects.active=o;bpy.ops.object.modifier_apply(modifier=mod.name)
for a in [0,2.1,4.2]:pole('feed arm',(1.1*math.cos(a),.28,2.9+1.1*math.sin(a)),(0,-1,2.9),.025,s)
box('radar console',(.95,-.5,.85),(.6,.5,.8),olive);save('RadarArray')
w,s,c,k=setup();olive=mat('Radar_Olive',(.1,.15,.08),.69,.35)
box('AA trailer',(0,0,.48),(3.4,2.2,.4),olive)
for x in [-1.1,1.1]:
 for y in [-1.12,1.12]:cylinder('AA wheel',(x,y,.39),.37,.24,k,rot=(math.pi/2,0,0))
cylinder('AA turntable',(0,0,.95),.65,.5,s)
for y in [-.75,-.25,.25,.75]:
 pole('launcher tube',(-1.3,y,1.1),(1.2,y,2.5),.2,olive);pole('dark launch opening',(1.18,y,2.49),(1.24,y,2.52),.16,k)
box('fire control unit',(-1.2,0,1.1),(.5,1.4,.55),olive);save('AirDefenseLauncher')
w,s,c,k=setup();steel=mat('Fuel_Silver',(.35,.36,.32),.42,.7);red=mat('Fuel_Red',(.34,.045,.02),.6,.5)
cylinder('fuel tank',(0,0,1.55),1.14,4.8,steel,rot=(0,math.pi/2,0))
for x in [-1.6,1.6]:box('tank cradle',(x,0,.47),(.3,2.5,.94),s)
for x in [-2.3,0,2.3]:ring('tank band',(x,0,1.55),1.15,.035,s,rot=(0,math.pi/2,0))
box('fuel pump',(3,-.5,.83),(.65,.55,1.66),red);pole('pump pipe',(2.35,0,1),(3,0,1),.05,s)
for i in range(5):ring('fuel hose',(3.2,-.9,.28+i*.08),.35,.032,k)
save('FuelStation')
(out/'regional_props_manifest.json').write_text(json.dumps(manifest,indent=2));print('REGIONAL_PROPS_READY',list(manifest),flush=True)
