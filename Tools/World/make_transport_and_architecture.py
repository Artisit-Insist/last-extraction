import bpy,math,json
from pathlib import Path
from mathutils import Vector
exec((Path(__file__).resolve().parent/'geometry_helpers.py').read_text())

def ellipsoid(name,p,s,m):
 bpy.ops.mesh.primitive_uv_sphere_add(segments=40,ring_count=24,location=p)
 o=bpy.context.object;o.name=name;o.scale=s;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);o.data.materials.append(m)
 for f in o.data.polygons:f.use_smooth=True
 return o

def skin(name,sections,m):
 verts=[];faces=[];n=24
 for x,ry,rz,z in sections:
  verts.extend([(x,ry*math.cos(i*math.tau/n),z+rz*math.sin(i*math.tau/n)) for i in range(n)])
 for j in range(len(sections)-1):
  for i in range(n):faces.append((j*n+i,j*n+(i+1)%n,(j+1)*n+(i+1)%n,(j+1)*n+i))
 faces.extend([tuple(reversed(range(n))),tuple((len(sections)-1)*n+i for i in range(n))])
 mesh=bpy.data.meshes.new(name);mesh.from_pydata(verts,[],faces);mesh.materials.append(m);mesh.update();o=bpy.data.objects.new(name,mesh);bpy.context.collection.objects.link(o)
 for f in mesh.polygons:f.use_smooth=True
 return o

clear();paint=mat('Transport_Olive',(.085,.11,.065),.48,.5);edge=mat('Transport_Edge',(.16,.19,.12),.5,.65);black=mat('Transport_Rubber',(.009,.012,.011),.85);glass=mat('Transport_Glass',(.026,.09,.105),.15,.55);seat=mat('Transport_Canvas',(.11,.14,.075),.9);red=mat('Transport_RescueMark',(.7,.035,.013),.5)
# Long open cabin, nose and tapering tail: all parts remain editable in the source.
nose_sections=[(1.9,1.3,1.05,1.6),(2.5,1.2,.94,1.52),(3.4,.82,.73,1.42),(4.05,.28,.38,1.32),(4.15,.05,.1,1.32)]
nose=skin('integrated glazed cockpit',nose_sections,paint);nose.data.materials.append(glass)
for f in nose.data.polygons:
 segment=f.index//24;angle=f.index%24
 if segment<3 and angle in [1,2,3,4,7,8,9,10]:f.material_index=1
for segment,(x,ry,rz,z) in enumerate(nose_sections[:4]):
 for i in range(1,12):
  a=i*math.tau/24;b=(i+1)*math.tau/24
  pole('window seal',(x,ry*math.cos(a)*1.006,z+rz*math.sin(a)*1.006),(x,ry*math.cos(b)*1.006,z+rz*math.sin(b)*1.006),.018,black)

skin('tail root',[(-2.1,1.28,1.0,1.58),(-3.1,.96,.76,1.76),(-4.0,.55,.47,1.99),(-8.8,.16,.22,2.78)],paint)
box('cabin floor',(-.1,0,.69),(4.5,2.62,.19),paint,bev=.09)
box('cabin ceiling',(-.1,0,2.68),(4.55,2.69,.22),paint,bev=.08)
for side in [-1,1]:
 for x in [-2.18,1.97]:box('door jamb',(x,side*1.32,1.72),(.15,.13,1.85),edge,bev=.045)
 box('upper door rail',(-.1,side*1.4,2.64),(4.55,.1,.12),edge)
 box('lower door rail',(-.1,side*1.4,.76),(4.55,.1,.1),edge)
 pole('boarding step',(-1.45,side*1.66,.39),(1.05,side*1.66,.39),.065,edge)
 for x in [-1.65,-.95,1.15]:
  box('troop seat cushion',(x,side*.8,.99),(.55,.53,.1),seat,bev=.06)
  box('seat back',(x,side*1.14,1.36),(.55,.1,.72),seat,bev=.045)
  for dx in [-.2,.2]:pole('seat frame',(x+dx,side*.8,.76),(x+dx,side*.8,.96),.018,edge)
  pole('seat restraint',(x-.2,side*1.075,1.6),(x+.2,side*.78,1.03),.012,black)
 pole('skid runner',(-2.4,side*1.66,.12),(2.55,side*1.66,.12),.09,edge)
 pole('skid raised nose',(2.55,side*1.66,.12),(3,side*1.66,.4),.09,edge)
 for x in [-1.55,1.45]:pole('landing strut',(x,side*.75,.68),(x,side*1.66,.14),.095,edge)
 for x in [-.4]:ellipsoid('turbine cover',(x,side*.54,3.03),(1.7,.43,.41),paint)
 cylinder('exhaust',(-2.14,side*.54,3.03),.25,.39,black,rot=(0,math.pi/2,0))
 box('tail horizontal stabilizer',(-7.0,side*.9,2.44),(1.3,1.8,.07),paint,rot=(0,.05,0),bev=.025)
 for x in [-1.9,-1.4,-.9,-.4,.1,.6,1.1,1.6]:
  for z in [.82,2.54]:ellipsoid('panel rivet',(x,side*1.37,z),(.022,.014,.022),edge)
box('tail fin',(-8.32,0,3.23),(.83,.12,2.37),paint,rot=(0,.24,0),bev=.12)
for x in [-1.6,.5]:
 for i in range(10):box('turbine cooling slots',(x+i*.05,-.945,3.07),(.025,.012,.28),black,bev=.004)
cylinder('rotor mast',(-.1,0,3.5),.13,.94,edge);ellipsoid('rotor hub',(-.1,0,3.99),(.39,.39,.16),edge)
pole('radio antenna',(-1.8,.38,3.24),(-2.25,.38,4.2),.012,black)
pole('nose probe',(4.05,0,1.25),(4.55,0,1.12),.025,edge)
for side in [-1,1]:
 box('rescue marking horizontal',(-3.0,side*.89,1.88),(.65,.028,.14),red)
 box('rescue marking vertical',(-3.0,side*.895,1.88),(.14,.029,.65),red)
save('RescueTransport')

clear();metal=mat('Transport_Rotor',(.015,.021,.017),.38,.6);tip=mat('Transport_RotorTip',(.49,.43,.18),.53,.4)
for i in range(4):
 angle=i*math.pi/2
 box('rotor blade',(3.7*math.cos(angle),3.7*math.sin(angle),0),(6.3,.33,.045),metal,rot=(.04,0,angle),bev=.01)
 box('blade tip',(6.74*math.cos(angle),6.74*math.sin(angle),0),(.24,.33,.047),tip,rot=(.04,0,angle),bev=.008)
 pole('blade pitch link',(.2*math.cos(angle),.2*math.sin(angle),-.15),(.65*math.cos(angle),.65*math.sin(angle),0),.035,metal)
save('TransportRotor')
clear();metal=mat('Transport_Rotor',(.015,.021,.017),.38,.6)
for i in range(4):
 a=i*math.pi/2;box('tail rotor blade',(.62*math.cos(a),0,.62*math.sin(a)),(1.18,.045,.14),metal,rot=(0,-a,0),bev=.012)
cylinder('tail hub',(0,0,0),.14,.18,metal,rot=(math.pi/2,0,0));save('TransportTailRotor')
clear();paint=mat('Transport_Olive',(.085,.11,.065),.48,.5);glass=mat('Transport_Glass',(.026,.09,.105),.15,.55)
box('sliding door lower',(0,0,.35),(2.02,.085,.69),paint,bev=.055)
for x in [-.96,.96]:box('door upright',(x,0,1.05),(.09,.085,.76),paint)
box('door upper',(0,0,1.47),(2.02,.085,.1),paint)
box('door window',(0,0,1.06),(1.83,.035,.73),glass,bev=.035);save('TransportDoor')

clear();stone=mat('Fortress_Concrete',(.26,.255,.215),.94);iron=mat('Field_Steel',(.045,.052,.045),.5,.6)
box('reinforced wall',(0,0,1.55),(8,.62,3.1),stone,bev=.07)
for x in [-4,0,4]:box('wall pier',(x,0,1.65),(.46,.88,3.3),stone,bev=.035)
for x in [-3.7,-2.5,-1.3,-.1,1.1,2.3,3.5]:
 pole('fence stanchion',(x,0,3.1),(x,0,3.95),.025,iron)
for z in [3.4,3.7,3.93]:pole('security wire',(-4,0,z),(4,0,z),.013,iron)
save('FortressWall')
clear();stone=mat('Fortress_Concrete',(.26,.255,.215),.94);iron=mat('Field_Steel',(.045,.052,.045),.5,.6)
box('bunker back',(0,2.1,1.6),(5.8,.55,3.2),stone);box('bunker roof',(0,0,3.23),(6.4,5.2,.52),stone,bev=.08)
for x in [-2.75,2.75]:box('bunker side',(x,0,1.6),(.45,4.5,3.2),stone)
for x in [-1.85,1.85]:
 box('bunker front lower',(x,-2.1,.6),(1.8,.5,1.2),stone);box('bunker front upper',(x,-2.1,2.68),(1.8,.5,1.0),stone)
box('door lintel',(0,-2.1,2.88),(2,.55,.7),stone)
save('ConcreteBunker')
clear();steel=mat('Hangar_Galvanized',(.22,.245,.24),.65,.55);dark=mat('Field_Steel',(.045,.052,.045),.5,.6)
for y in [-5.9,5.9]:
 for x in [-7.0,7.0]:box('hangar pillar',(x,y,3.4),(.28,.28,6.8),dark)
for i in range(56):
 x=-6.875+i*.25
 for y in [-6,6]:box('corrugated side',(x,y,2.5),(.27,.045,5),steel,bev=.005)
for i in range(48):box('hangar rear',(-7,-5.875+i*.25,2.5),(.045,.27,5),steel,bev=.005)
for i in range(32):
 a=i*math.pi/32;b=(i+1)*math.pi/32
 y=6*math.cos((a+b)/2);z=5+2.1*math.sin((a+b)/2)
 box('curved roof',(0,y,z),(14.5,.62,.07),steel,rot=((a+b)/2-math.pi/2,0,0),bev=.01)
save('FieldHangar')
(out/'world_revision_manifest.json').write_text(json.dumps(manifest,indent=2));print('TRANSPORT_AND_ARCHITECTURE_READY',list(manifest),flush=True)
