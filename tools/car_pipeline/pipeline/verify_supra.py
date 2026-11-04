from pathlib import Path
from PIL import Image,ImageDraw
import numpy as np,json,hashlib,time
import sys as _path_sys; _path_sys.path.insert(0,str(Path(__file__).resolve().parent))
from settings import WORKSPACE as r, CODE, GAME, SOURCES;g=GAME/'assets/cars/toyota_supra_a80_sz';start=time.perf_counter();m=json.loads((g/'manifest.json').read_text());assert len(m['files'])==576
assert sum(name.endswith('_rim_mask.png') for name in m['files'])==54
for name,digest in m['files'].items():
 p=g/name;assert hashlib.sha256(p.read_bytes()).hexdigest()==digest,name;assert Image.open(p).size==(128,128)
def load(prefix,k):return np.array(Image.open(g/'sprites'/f'{prefix}_{k}.png'))
sheet=Image.new('RGBA',(6*256,2*256),'#202832');mask_counts=[]
for column,(view,pose) in enumerate((v,p) for v in ['front','back'] for p in ['left','straight','right']):
 label=f'{view}_{pose}';base=load(label+'_body','albedo');wheel=load(label+'_wheels_f0','albedo');wd=load(label+'_wheels_f0','pack').astype(int);wheeldepth=wd[...,0]*256+wd[...,1]
 for sample in ['', '_pitch_m2','_pitch_m1','_pitch_p1','_pitch_p2']:
  prefix=label+'_body'+sample;alb=load(prefix,'albedo');mask=load(prefix,'mask');dep=load(prefix,'pack').astype(int);far=load(prefix,'pack2').astype(int);emit=load(prefix,'emit');a=alb[...,3]>127
  box=Image.fromarray(alb).getchannel('A').getbbox();assert box and min(box[:2])>0 and max(box[2:])<128,(prefix,'Clipped silhouette',box)
  assert np.all((far[...,1]*256+far[...,2])[a]<=(dep[...,0]*256+dep[...,1])[a]),prefix
  assert np.count_nonzero(mask[...,0])>100 and np.count_nonzero(mask[...,2])>10,(prefix,'Missing paint/hood')
  white=np.all(alb[...,:3]==255,axis=-1)&a;glass=np.all(alb[...,:3]==[66,69,75],axis=-1)&a
  assert not mask[white|glass,:3].any(),(prefix,'Glass/plate painted')
  if sample:assert not np.array_equal(alb,base),(prefix,'Pitch absent')
  if view=='front':assert np.any(emit[...,0]>16),(prefix,'Missing headlight')
  else:assert np.any(emit[...,1]>16),(prefix,'Missing tail light')
  assert np.any(emit[...,2]>16),(prefix,'Missing indicator')
  yy,xx=np.mgrid[:128,:128];cx=(xx+.5-64)/26;cy=(79-yy-.5)/26;cz=((dep[...,0]*256+dep[...,1])/65535-.5)*5.2
  side=(1 if view=='front' else -1)*(.70710678*cx+.35355339*cy-.61237244*cz)
  amber=emit[...,2]>16
  # Far-side front lenses can self-occlude or fall below one native pixel.
  # The rear quarter exposes both sides and must retain both independent signals.
  if view=='back':assert np.any(amber&(side<0)) and np.any(amber&(side>0)),(prefix,'Missing one indicator side')
  near=np.max(np.abs(alb[...,:3].astype(int)-[219,229,237]),axis=-1)<2
  assert not np.any(near&a&~(mask[...,:3].max(-1)>127)),(prefix,'Unmasked body paint')
  mask_counts.append(int((mask[...,:3].max(-1)>127).sum()))
 for row,colour in enumerate([[174,181,184],[28,31,36]]):
  alb=base.copy();mask=load(label+'_body','mask');paint=mask[...,:3].max(-1)>127;alb[paint,:3]=np.clip(alb[paint,:3]*np.array(colour)/[219,229,237],0,255)
  bd=load(label+'_body','pack').astype(int);bodydepth=bd[...,0]*256+bd[...,1];take=(wheel[...,3]>127)&((alb[...,3]<128)|(wheeldepth>bodydepth));alb[take]=wheel[take]
  sheet.alpha_composite(Image.fromarray(alb).resize((256,256),Image.Resampling.NEAREST),(column*256,row*256))
sheet.convert('RGB').save(r/'output/supra-review.png');print('PASS SUPRA 576 hashes/canvases, 54 rim masks, 30 body poses, paint/hood/glass/plates, lamps, depth and silhouette margins',flush=True)
