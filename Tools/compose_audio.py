import numpy as np,wave,json
from pathlib import Path
P=(Path(__file__).resolve().parent.parent/'ArtSource'/'Audio');P.mkdir(parents=True,exist_ok=True)
SR=32000;rng=np.random.default_rng(1985)
def save(name,x):
 x=np.asarray(x);peak=np.max(np.abs(x));x=x/max(1,peak/0.92);x=(x*32767).astype('<i2')
 with wave.open(str(P/(name+'.wav')),'wb') as f:f.setnchannels(1 if x.ndim==1 else 2);f.setsampwidth(2);f.setframerate(SR);f.writeframes(x.tobytes())
def tone(freq,dur):
 t=np.arange(int(dur*SR))/SR
 return t,np.sin(2*np.pi*freq*t)
def noise(dur):return rng.normal(0,1,int(dur*SR))
# Effects: original layered synthesis, no sampled commercial recordings.
t,s=tone(75,.32);n=noise(.32);save('Rifle',(.48*n*np.exp(-t*48)+.58*np.sin(2*np.pi*(95*t-48*t*t))*np.exp(-t*24)))
t,s=tone(680,.32);save('Arrow',noise(.32)*np.exp(-t*17)*(.18+.16*np.sin(t*70))+s*.08*np.exp(-t*20))
t,s=tone(48,2.4);n=noise(2.4);low=np.convolve(n,np.ones(45)/45,'same');save('Explosion',n*.31*np.exp(-t*8)+low*2.3*np.exp(-t*1.9)+s*.37*np.exp(-t*2.4))
t,s=tone(220,.24);save('Hit',noise(.24)*.28*np.exp(-t*24)+s*.18*np.exp(-t*20))
t=np.arange(int(.8*SR))/SR;x=np.zeros_like(t)
for k,f in enumerate([440,659.255,880]):
 q=t-k*.10;x+=np.where(q>=0,np.sin(2*np.pi*f*q)*np.exp(-np.maximum(q,0)*7),0)*.18
save('Pickup',x)
# Stereo ambient field: filtered rain/wind, insects and sparse birds.
dur=64;t=np.arange(int(dur*SR))/SR;n=noise(dur);wind=np.convolve(n,np.ones(300)/300,'same')*.5
forest=np.column_stack([wind.copy(),wind.copy()]);
for i in range(110):
 start=rng.uniform(0,dur-1);d=rng.uniform(.18,.6);tt=np.arange(int(d*SR))/SR;freq=rng.uniform(1700,3300);chirp=np.sin(2*np.pi*(freq*tt+700*tt*tt))*np.sin(np.pi*tt/d)**3*.025
 j=int(start*SR);forest[j:j+len(tt),i%2]+=chirp
forest[:,0]+=np.sin(2*np.pi*4700*t)*(.018+.006*np.sin(t*21));forest[:,1]+=np.sin(2*np.pi*4821*t)*(.018+.006*np.sin(t*19));fade=np.minimum(1,t/1.5)*np.minimum(1,(dur-t)/1.5);save('Forest',forest*fade[:,None])
# 80 BPM, D minor, 64 bars / 192 seconds. Four evolving 16-bar movements.
bpm=80;beat=60/bpm;bars=64;dur=bars*4*beat;N=int(dur*SR);music=np.zeros((N,2),dtype=np.float64)
def add(x,start,pan=0,gain=1):
 i=int(start*SR);a=max(0,min(len(x),N-i));
 if a:music[i:i+a,0]+=x[:a]*gain*(.7-pan*.3);music[i:i+a,1]+=x[:a]*gain*(.7+pan*.3)
def note(midi,start,dur,gain=.1,pan=0,kind='pad'):
 f=440*2**((midi-69)/12);tt=np.arange(int(dur*SR))/SR
 if kind=='pad':x=(np.sin(2*np.pi*f*tt)+.3*np.sin(2*np.pi*f*1.002*tt)+.17*np.sin(2*np.pi*f*2*tt));env=np.minimum(tt/.6,1)*np.minimum((dur-tt)/.9,1)
 elif kind=='bell':x=np.sin(2*np.pi*f*tt)+.3*np.sin(2*np.pi*f*2.76*tt);env=np.exp(-tt*2.2)*np.minimum(tt/.008,1)
 else:x=np.sin(2*np.pi*f*tt)+.15*np.sin(2*np.pi*f*2*tt);env=np.exp(-tt*3)*np.minimum(tt/.008,1)
 add(x*env,start,pan,gain)
chords=[[38,50,57,62,65],[34,46,53,58,62],[41,53,60,65,69],[36,48,55,60,64]]
melody=[74,69,72,65,67,69,62,65,74,77,76,72,69,67,65,62]
for bar in range(bars):
 base=bar*4*beat;phase=bar//16;ch=chords[(bar//2)%4]
 for j,m in enumerate(ch):note(m,base,3.4,.016 if j else .035,(j-2)/3,'pad')
 if phase!=0 or bar>=8:
  for k in range(8):note(ch[0]+12,base+k*beat/2,.4,.065 if k%2==0 else .035,(-1)**k*.2,'bass')
 if phase in [1,2,3]:
  for k in [0,2]:
   tt=np.arange(int(.4*SR))/SR;kick=np.sin(2*np.pi*(52*tt+3*(1-np.exp(-tt*40))))*np.exp(-tt*12);add(kick,base+k*beat,0,.13)
  for k in [1,3]:
   tt=np.arange(int(.18*SR))/SR;hit=rng.normal(0,1,len(tt))*np.exp(-tt*35);add(hit,base+k*beat,.1,.04)
 if phase==2:
  for k in range(8):
   tt=np.arange(int(.07*SR))/SR;add(rng.normal(0,1,len(tt))*np.exp(-tt*65),base+k*beat/2,(-1)**k*.5,.018)
 if bar%2==0:
  for k in range(4):note(melody[(bar//2+k)%len(melody)],base+k*beat,2.1,.055,(-1)**k*.4,'bell')
# A short stereo echo glues the score, limiting without hard clipping.
for channel,delay in [(0,.375),(1,.5625)]:
 d=int(delay*SR);music[d:,channel]+=music[:-d,channel]*.23
music=np.tanh(music*1.6)*.72;fade=np.minimum(1,np.arange(N)/SR/3)*np.minimum(1,(N-np.arange(N))/SR/4);save('JungleScore',music*fade[:,None])
manifest={p.name:{'seconds':wave.open(str(p)).getnframes()/SR,'sample_rate':SR} for p in P.glob('*.wav')}
(P/'audio_manifest.json').write_text(json.dumps(manifest,indent=2));print(json.dumps(manifest))
