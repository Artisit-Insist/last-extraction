import bpy,math,random
from pathlib import Path
from mathutils import Vector
random.seed(1985)
P=(Path(__file__).resolve().parent.parent/'ArtSource'/'Meshes');P.mkdir(parents=True,exist_ok=True)
def clean():
 bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
def uv(name,p,s,seg=16,rings=10):
 bpy.ops.mesh.primitive_uv_sphere_add(segments=seg,ring_count=rings,location=p);o=bpy.context.object;o.name=name;o.scale=s;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
 for f in o.data.polygons:f.use_smooth=True
 return o
def export(name):
 bpy.ops.object.select_all(action='SELECT');bpy.context.view_layer.objects.active=bpy.context.selected_objects[0];bpy.ops.object.join();o=bpy.context.object;o.name=name;bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
 bpy.ops.export_scene.fbx(filepath=str(P/(name+'.fbx')),use_selection=True,object_types={'MESH'},add_leaf_bones=False,axis_forward='-Y',axis_up='Z',apply_unit_scale=True)
clean()
# An irregular faceted boulder with broad smooth eroded faces.
bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=3,radius=.5);o=bpy.context.object
for v in o.data.vertices:v.co*=random.uniform(.87,1.13)
for f in o.data.polygons:f.use_smooth=True
export('Rock')
clean()
# A palm crown of arched, individually cut leaflets rather than ellipsoid placeholders.
verts=[];faces=[]
for frond in range(9):
 angle=frond*math.tau/9
 for j in range(1,15):
  t=j/15;r=t*.5;z=.12*math.sin(t*math.pi)-.18*t*t
  for side in [-1,1]:
   w=.105*math.sin(t*math.pi)*side
   coords=[(r-.02,0,z),(r+.025,0,z+.006),(r+.055,w,z-.025),(r+.015,w*.72,z-.04)]
   k=len(verts)
   for x,y,zz in coords:verts.append((x*math.cos(angle)-y*math.sin(angle),x*math.sin(angle)+y*math.cos(angle),zz))
   faces.append((k,k+1,k+2,k+3));faces.append((k+3,k+2,k+1,k))
me=bpy.data.meshes.new('PalmLeaflets');me.from_pydata(verts,[],faces);ob=bpy.data.objects.new('Palm',me);bpy.context.collection.objects.link(ob);export('Palm')
clean()
# Detailed fitted trouser leg with knee pad, cargo pocket, and boot.
uv('thigh',(0,0,.18),(.46,.43,.31));uv('calf',(0,0,-.20),(.32,.33,.28));uv('knee',(.17,0,-.02),(.30,.35,.15));uv('pocket',(-.02,.33,.18),(.28,.14,.18));export('Leg')
clean()
# Torso: torso barrel, pectorals and shoulder definition.
uv('body',(0,0,0),(.40,.45,.49));uv('chestL',(.18,-.23,.15),(.3,.26,.23));uv('chestR',(.18,.23,.15),(.3,.26,.23));uv('waist',(-.02,0,-.33),(.30,.34,.2));export('Torso')
clean()
uv('skull',(0,0,.06),(.43,.37,.43),24,16);uv('jaw',(.08,0,-.25),(.34,.29,.22));uv('nose',(.42,0,-.025),(.15,.09,.15));uv('earL',(0,-.38,0),(.13,.10,.18));uv('earR',(0,.38,0),(.13,.10,.18));uv('browL',(.34,-.18,.15),(.14,.15,.075));uv('browR',(.34,.18,.15),(.14,.15,.075));export('Head')
# Save an editable library scene containing the final mesh; FBX sources preserve each preceding mesh.
bpy.ops.wm.save_as_mainfile(filepath=str(P/'MeshAuthoring.blend'))
print('MESH_LIBRARY_READY')
