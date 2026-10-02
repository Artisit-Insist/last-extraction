import bpy,math,random,json
from pathlib import Path
from mathutils import Vector
root=Path(__file__).resolve().parents[2]/'ArtSource/Production';out=root/'Export';random.seed(17);manifest={}
def clear():bpy.ops.wm.read_factory_settings(use_empty=True)
def mat(n,c,r=.7,metal=0):
 m=bpy.data.materials.new(n);m.diffuse_color=(*c,1);m.use_nodes=True;p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=(*c,1);p.inputs['Roughness'].default_value=r;p.inputs['Metallic'].default_value=metal;return m
def box(n,p,s,m,rot=(0,0,0),bev=.015):
 bpy.ops.mesh.primitive_cube_add(size=1,location=p,rotation=rot);o=bpy.context.object;o.name=n;o.scale=s;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);o.data.materials.append(m)
 if bev:
  mod=o.modifiers.new('Worn edges','BEVEL');mod.width=bev;mod.segments=2;bpy.ops.object.modifier_apply(modifier=mod.name)
 return o
def cylinder(n,p,r,d,m,rot=(0,0,0)):
 bpy.ops.mesh.primitive_cylinder_add(vertices=20,radius=r,depth=d,location=p,rotation=rot);o=bpy.context.object;o.name=n;o.data.materials.append(m);mod=o.modifiers.new('Rim','BEVEL');mod.width=.003;mod.segments=2;bpy.ops.object.modifier_apply(modifier=mod.name);return o
def pole(n,a,b,r,m):
 a,b=Vector(a),Vector(b);o=cylinder(n,(a+b)*.5,r,(b-a).length,m);o.rotation_euler=(b-a).to_track_quat('Z','Y').to_euler();return o
def save(n):
 obs=[o for o in bpy.context.scene.objects if o.type=='MESH'];bpy.ops.object.select_all(action='DESELECT')
 for o in obs:o.select_set(True)
 bpy.context.view_layer.objects.active=obs[0];bpy.ops.object.join();o=bpy.context.object;o.name=n;bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
 bpy.ops.object.shade_smooth_by_angle(angle=math.radians(45),keep_sharp_edges=True)
 bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT');bpy.ops.uv.smart_project(island_margin=.015);bpy.ops.object.mode_set(mode='OBJECT')
 mats=[]
 for m in o.data.materials:
  bs=m.node_tree.nodes.get('Principled BSDF');info={'name':m.name,'base':list(m.diffuse_color),'roughness':bs.inputs['Roughness'].default_value,'metallic':bs.inputs['Metallic'].default_value,'textures':{}}
  if m.name=='Field_Canvas':info['textures']={'Camouflage_diff_2k.png':str(root/'Camouflage.png')}
  if m.name=='Field_Timber':
   p=next((root/'wooden_crate_02/textures').glob('*_diff_*.jpg'));info['textures'][p.name]=str(p)
  mats.append(info)
 manifest[n]={'materials':mats,'source':'Original authored geometry','size_m':list(o.dimensions)}
 bpy.ops.export_scene.fbx(filepath=str(out/(n+'.fbx')),use_selection=True,object_types={'MESH'},axis_forward='-Y',axis_up='Z',path_mode='ABSOLUTE')
 bpy.ops.wm.save_as_mainfile(filepath=str(root/(n+'.blend')))
