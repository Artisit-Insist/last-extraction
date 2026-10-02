"""CMU ASF/AMC forward kinematics. Source: mocap.cs.cmu.edu/info.php.
Lengths: inches / 0.45; rotations: ASF axis conjugation around AMC channels.
"""
import math
from pathlib import Path
from mathutils import Vector,Matrix,Euler
SCALE=.0254/.45
CONVERT=Matrix(((1,0,0),(0,0,-1),(0,1,0)))
def rotation(angles):return Euler(tuple(math.radians(v) for v in angles),'XYZ').to_matrix()
def read_asf(path):
 bones={};parents={};mode=None;cur=None
 for raw in Path(path).read_text().splitlines():
  a=raw.split()
  if not a:continue
  if a[0].startswith(':'):mode=a[0];continue
  if mode==':bonedata':
   if a[0]=='begin':cur={}
   elif a[0]=='end':bones[cur['name']]=cur
   elif a[0]=='name':cur['name']=a[1]
   elif a[0]=='direction':cur['direction']=Vector(map(float,a[1:4]))
   elif a[0]=='length':cur['length']=float(a[1])*SCALE
   elif a[0]=='axis':cur['axis']=rotation(list(map(float,a[1:4])))
   elif a[0]=='dof':cur['dof']=a[1:]
  elif mode==':hierarchy' and a[0] not in ['begin','end']:
   for child in a[1:]:parents[child]=a[0]
 return bones,parents
def read_amc(path):
 frames=[];frame=None
 for raw in Path(path).read_text().splitlines():
  a=raw.split()
  if not a or a[0][0] in '#:':continue
  if a[0].isdigit():frame={};frames.append(frame)
  else:frame[a[0]]=list(map(float,a[1:]))
 return frames
def fk(bones,parents,frame):
 r=frame['root'];positions={'root':Vector(r[:3])*SCALE};rotations={'root':rotation(r[3:6])}
 def calc(name):
  if name in positions:return
  parent=parents[name];calc(parent);b=bones[name];angles={'rx':0,'ry':0,'rz':0};angles.update(zip(b.get('dof',[]),frame.get(name,[])));C=b['axis'];R=rotations[parent]@C@rotation([angles['rx'],angles['ry'],angles['rz']])@C.inverted()
  rotations[name]=R;positions[name]=positions[parent]+R@(b['direction']*b['length'])
 for n in bones:calc(n)
 return {n:CONVERT@p for n,p in positions.items()},{n:CONVERT@R@CONVERT.transposed() for n,R in rotations.items()}
