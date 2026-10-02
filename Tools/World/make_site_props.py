"""Create distinct working/living spaces from original editable geometry."""
import bpy,math,json,random
from pathlib import Path
from mathutils import Vector
exec((Path(__file__).resolve().parent/'geometry_helpers.py').read_text())
root=Path(__file__).resolve().parents[2]/'ArtSource/Production';out=root/'Props08';out.mkdir(exist_ok=True);random.seed(2808)
# Keep geometry_helpers exports isolated from earlier authored models.
_original_save=save
def finish(name):
 _original_save(name)
 source=root/(name+'.blend');target=out/(name+'.blend')
 source.replace(target)
 manifest[name]['blend']=str(target.relative_to(root));manifest[name]['fbx']=str((out/(name+'.fbx')).relative_to(root))
def setup():
 clear()
 return {'wood':mat('Field_Timber',(.2,.13,.065),.87),'steel':mat('Field_Steel',(.1,.13,.11),.58,.4),'canvas':mat('Canvas_Tan',(.36,.29,.15),.96),'olive':mat('Mission_Olive',(.13,.18,.085),.8),'rust':mat('Mine_Rust',(.3,.12,.045),.86,.2),'rubber':mat('Field_Rubber',(.019,.019,.016),.92),'cream':mat('Washed_Cloth_Cream',(.53,.48,.35),.96),'blue':mat('Washed_Cloth_Blue',(.12,.21,.25),.94),'brick':mat('Village_Plaster',(.31,.21,.13),.98)}
def ring(n,p,r,t,m,rot=(0,0,0)):
 bpy.ops.mesh.primitive_torus_add(major_radius=r,minor_radius=t,major_segments=32,minor_segments=8,location=p,rotation=rot);o=bpy.context.object;o.name=n;o.data.materials.append(m);return o
def cloth(n,center,size,m,sag=.18):
 cx,cy,cz=center;w,d=size;vs=[];fs=[]
 for j in range(13):
  y=(j/12-.5)*d
  for i in range(25):
   x=(i/24-.5)*w;z=cz+.42*(1-abs(x/(w*.5)))-sag*math.sin(math.pi*j/12)+.025*math.sin(i*1.1+j*.5);vs.append((cx+x,cy+y,z))
 for j in range(12):
  for i in range(24):a=j*25+i;fs.append((a,a+1,a+26,a+25))
 me=bpy.data.meshes.new(n);me.from_pydata(vs,[],fs);me.materials.append(m);o=bpy.data.objects.new(n,me);bpy.context.collection.objects.link(o)
 for f in me.polygons:f.use_smooth=True
 sol=o.modifiers.new('Canvas thickness','SOLIDIFY');sol.thickness=.013;bpy.context.view_layer.objects.active=o;bpy.ops.object.modifier_apply(modifier=sol.name)
 return o
def canopy(m,w=5,d=3.8,z=2.35):
 cloth('stretched fabric with sag',(0,0,z),(w,d),m['canvas'])
 for x in [-w/2,w/2]:
  for y in [-d/2,d/2]:
   pole('tent upright',(x,y,0),(x,y,z),.048,m['wood']);pole('guy rope',(x,y,z-.08),(x*1.14,y*1.14,.06),.011,m['cream']);pole('stake',(x*1.14,y*1.14,-.06),(x*1.14,y*1.14,.20),.022,m['steel'])
 for y in [-d/2,d/2]:pole('ridge post',(0,y,0),(0,y,z+.4),.052,m['wood'])
def cot(m,p):
 x,y=p;box('canvas cot',(x,y,.52),(2,.76,.045),m['olive'],bev=.025)
 for yy in [-.41,.41]:pole('cot frame',(x-1.05,y+yy,.53),(x+1.05,y+yy,.53),.022,m['steel'])
 for xx in [-.76,.76]:
  for yy in [-.31,.31]:pole('folding cot leg',(x+xx,y+yy,0),(x+xx*.7,y-yy,.5),.022,m['steel'])
 box('folded pillow',(x-.75,y,.62),(.4,.58,.15),m['cream'],bev=.05)
def jerry(m,p):
 x,y,z=p;box('pressed fuel can',(x,y,z+.27),(.34,.19,.53),m['olive'],bev=.055);pole('can handle',(x-.1,y,z+.55),(x+.1,y,z+.55),.023,m['steel']);cylinder('fuel cap',(x+.105,y,z+.55),.039,.025,m['steel'])
 for a,b in [((x-.12,y-.105,z+.1),(x+.12,y-.105,z+.42)),((x+.12,y-.105,z+.1),(x-.12,y-.105,z+.42))]:pole('pressed cross rib',a,b,.009,m['steel'])
def chest(m,p,w=.9):
 x,y,z=p;box('tool chest',(x,y,z+.28),(w,.55,.56),m['olive'],bev=.035);box('chest lid',(x,y,z+.58),(w+.025,.575,.04),m['steel'])
 for xx in [-w*.3,w*.3]:box('hinge and latch',(x+xx,y-.29,z+.46),(.07,.03,.19),m['steel'],bev=.004)
def worktable(m,p=(0,0),w=2,d=.85):
 x,y=p;box('scarred work top',(x,y,.92),(w,d,.10),m['wood'])
 for xx in [-w*.43,w*.43]:
  for yy in [-d*.38,d*.38]:box('table leg',(x+xx,y+yy,.46),(.08,.08,.92),m['wood'])
 box('low storage shelf',(x,y,.22),(w*.92,d*.85,.055),m['wood'])
def pot(m,p,r=.25):
 x,y,z=p;vs=[];fs=[]
 for row,(rr,zz) in enumerate([(r*.6,0),(r*.92,.08),(r,.3),(r*.7,.46),(r*.65,.5),(r*.53,.5),(r*.57,.40)]):
  for j in range(32):a=j*math.tau/32;vs.append((x+rr*math.cos(a),y+rr*math.sin(a),z+zz))
 for row in range(6):
  for j in range(32):a=row*32+j;b=row*32+(j+1)%32;fs.append((a,b,b+32,a+32))
 me=bpy.data.meshes.new('clay vessel');me.from_pydata(vs,[],fs);me.materials.append(m['brick']);ob=bpy.data.objects.new('handmade clay pot',me);bpy.context.collection.objects.link(ob)
 for f in me.polygons:f.use_smooth=True

m=setup();canopy(m)
for y in [-.8,.8]:cot(m,(0,y))
for i in range(3):jerry(m,(1.5+i*.38,1.25,0))
chest(m,(-1.55,1.3,0));ring('rolled sleeping mat',(-1.55,-1.1,.32),.25,.07,m['canvas'],(math.pi/2,0,0));finish('JungleBivouac08')

m=setup();m['canvas']=m['cream'];canopy(m,4.6,3.4,2.55);cot(m,(0,.65));worktable(m,(0,-.9),1.9,.65)
for x in [-.6,0,.6]:box('bandage pack',(x,-.9,1.045),(.32,.23,.14),m['cream'],bev=.04)
pole('saline stand',(1.5,.5,0),(1.5,.5,2.05),.016,m['steel'])
for a in [0,2.1,4.2]:pole('IV tripod',(1.5,.5,.15),(1.5+.38*math.cos(a),.5+.38*math.sin(a),.02),.014,m['steel'])
box('saline pouch',(1.4,.5,1.8),(.16,.07,.27),m['cream'],bev=.04);pole('IV tube',(1.4,.5,1.65),(.5,.65,.6),.004,m['rubber']);chest(m,(-1.55,-.9,0),.7);finish('FieldClinic08')

m=setup();worktable(m,(0,0),2.8,1.0)
for x in [-1.8,1.8]:
 pole('boat repair rack',(x,-.6,0),(x,-.6,1.9),.05,m['wood']);pole('crossbar',(x,-.75,1.55),(x,.65,1.55),.045,m['wood'])
for i in range(3):
 y=-.4+i*.35;pole('oar shaft',(-2,y,1.6),(2,y,1.6),.021,m['wood']);box('oar blade',(1.68,y,1.6),(.72,.22,.04),m['wood'],bev=.06)
box('outboard engine',(.4,0,1.25),(.45,.55,.56),m['steel'],bev=.1);pole('propeller shaft',(.4,.2,.96),(.4,.5,.55),.05,m['steel']);ring('life ring',(-1.25,.65,1.1),.36,.065,m['canvas'],(math.pi/2,0,0))
for i in range(5):ring('mooring rope coil',(-1,0,.99+i*.014),.22+i*.012,.011,m['cream'])
chest(m,(.85,-.6,0),.6);finish('BoatRepair08')

m=setup();box('masonry stove',(-.85,.3,.32),(1.0,.9,.64),m['brick'],bev=.05)
for x in [-1.1,-.6]:ring('stove iron burner',(x,.3,.66),.19,.023,m['steel']);pot(m,(-.85,.3,.66),.27)
for i in range(9):pole('split firewood',(.25+(i%3)*.17,.1,(i//3)*.13+.1),(.25+(i%3)*.17,.75,(i//3)*.13+.1),.067,m['wood'])
worktable(m,(.45,-.8),1.6,.65)
for x in [0,.5,.95]:pot(m,(x,-.8,.97),.18)
for x in [-1.3,1.4]:pot(m,(x,1.0,0),.30)
pole('drying rail',(-1.6,1.3,1.7),(1.6,1.3,1.7),.025,m['wood'])
for x in [-1.6,1.6]:pole('rail support',(x,1.3,0),(x,1.3,1.7),.035,m['wood'])
for x in [-.8,0,.8]:ring('hanging pan',(x,1.28,1.35),.19,.028,m['steel'],(math.pi/2,0,0))
finish('VillageKitchen08')

m=setup();worktable(m,(0,0),3,.95);box('tool board',(0,.58,1.55),(3,.07,1.35),m['wood'])
for i in range(9):
 x=-1.2+i*.3;pole('tool handle',(x,.5,1.15),(x,.5,1.55),.02,m['wood']);box('hammer head',(x,.5,1.59),(.16,.07,.07),m['steel'])
box('bench vise',(-.7,-.15,1.08),(.4,.28,.25),m['steel']);pole('vise screw',(-.7,-.4,1.08),(-.7,-.08,1.08),.014,m['steel'])
for x in [1.9,2.28]:cylinder('gas bottle',(x,.3,.72),.17,1.32,m['rust']);cylinder('valve',(x,.3,1.45),.033,.14,m['steel']);ring('valve wheel',(x,.3,1.53),.07,.014,m['steel'])
for i in range(6):ring('welding hose',(1.65,-.6,.06+i*.036),.38,.018,m['rubber'])
chest(m,(-1.65,-.55,0));finish('QuarryWorkbench08')

m=setup();worktable(m,(0,0),2.6,1.15);box('field map board',(0,.25,1.23),(2.05,.07,1.15),m['wood'],rot=(.45,0,0));box('map sheet',(0,.20,1.23),(1.84,.018,.94),m['cream'],rot=(.45,0,0))
for i in range(6):pole('map route markers',(-.8+i*.25,-.02,.95),(-.65+i*.25,.17,1.3),.006,m['olive'])
box('radio set',(-.65,-.24,1.1),(.66,.42,.35),m['olive'])
for i in range(5):box('speaker grille',(-.9+i*.065,-.46,1.12),(.02,.01,.2),m['steel'])
for x in [-.5,-.34]:cylinder('radio dial',(x,-.465,1.05),.039,.04,m['steel'],(math.pi/2,0,0))
pole('radio antenna',(-.85,-.15,1.28),(-.85,-.15,2.55),.008,m['steel']);chest(m,(1.65,.1,0),.75);finish('CommandTable08')

m=setup()
for x in [-1.7,1.7]:
 for y in [-.85,.85]:cylinder('cargo trolley tire',(x,y,.28),.26,.14,m['rubber'],(math.pi/2,0,0));pole('axle',(x,-.9,.28),(x,.9,.28),.04,m['steel'])
box('cargo trolley deck',(0,0,.49),(4,1.9,.14),m['steel'])
for y in [-.9,.9]:pole('low deck rail',(-2,y,.65),(2,y,.65),.03,m['steel'])
for x,y,z,w in [(-1.15,-.3,.56,1.2),(.45,.15,.56,1.6),(-1.15,-.3,1.25,1.05)]:box('freight case',(x,y,z+.34),(w,1.05,.68),m['wood'],bev=.035)
cloth('freight tarpaulin',(.5,0,1.65),(2.1,1.5),m['blue'],.06)
for x in [-1.4,-.9,.3,.8]:
 pole('cargo restraint',(x,-.83,.55),(x,-.65,1.9),.022,m['canvas']);pole('cargo tie',(x,-.65,1.9),(x,.65,1.9),.022,m['canvas']);pole('cargo restraint',(x,.65,1.9),(x,.83,.55),.022,m['canvas'])
pole('towing handle',(2,0,.5),(3.1,0,.24),.033,m['steel']);ring('tow ring',(3.2,0,.24),.12,.025,m['steel']);finish('AirCargoTrolley08')

m=setup();char=mat('Charred_Wood08',(.045,.031,.021),.98)
for x in [-2.2,2.2]:
 for y in [-1.6,1.6]:box('burned upright',(x,y,1.1 if x<0 else .7),(.17,.17,2.2 if x<0 else 1.4),char,rot=(.05,.07,0))
for i in range(11):
 x=-1.9+i*.38;box('splintered wall plank',(x,1.5,.5+.15*math.sin(i)),(.31,.08,1+.3*math.sin(i)),char,rot=(0,.025*i,0),bev=.007)
for i in range(6):box('fallen rafters',(-1.4+i*.55,0,.18),(2.5,.12,.13),char,rot=(0,.08,i*.7),bev=.01)
for i in range(8):box('collapsed sheet metal',(.45+(i%4)*.36,-.8+(i//4)*.75,.24+i*.025),(.5,1.9,.035),m['rust'],rot=(0,.10,(i-3)*.08),bev=.01)
finish('BurntShelter08')
# Preserve the earlier fuel station and export a separate copy with smooth tank normals.
bpy.ops.wm.open_mainfile(filepath=str(root/'FuelStation.blend'));finish('FuelStation08')
manifest['FuelStation08']['source']='Refined shading of the previously authored FuelStation; original retained'
(out/'site_props_manifest.json').write_text(json.dumps(manifest,indent=2));print('SITE_PROPS08_READY',list(manifest),flush=True)
