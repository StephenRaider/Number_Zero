"""Validate the imported R34 runtime maps, including every pitch and wheel state."""
from pathlib import Path
import hashlib,json
import numpy as np
from PIL import Image
root=Path(__file__).resolve().parents[1]/'assets/cars/nissan_skyline_r34_gtr'
m=json.loads((root/'manifest.json').read_text())
assert len(m['files'])==576
for name,digest in m['files'].items():
 p=root/name;assert hashlib.sha256(p.read_bytes()).hexdigest()==digest,name
 im=Image.open(p);im.load();assert im.size==(128,128),name
rim_count=0
for label,pose in m['poses'].items():
 assert len(pose['stock_wheels']['frames'])==9
 for f in pose['stock_wheels']['frames']:
  rim=np.array(Image.open(root/f['rim_mask']));alpha=np.array(Image.open(root/f['albedo']).convert('RGBA'))[...,3]
  assert rim.shape==(128,128) and rim.any() and not rim[alpha==0].any(),f['rim_mask'];rim_count+=1
 for sample in ['', '_pitch_m2','_pitch_m1','_pitch_p1','_pitch_p2']:
  stem=root/'sprites'/(label+'_body'+sample)
  read=lambda suffix:np.array(Image.open(str(stem)+'_'+suffix+'.png').convert('RGBA'))
  a,mask,depth,back,emit=[read(k) for k in ['albedo','mask','pack','pack2','emit']]
  covered=a[...,3]>127
  assert covered.any() and (mask[...,0]>127).sum()>100 and (mask[...,2]>127).sum()>10,stem
  white=np.all(a[...,:3]==255,-1)&covered;glass=np.all(a[...,:3]==[66,69,75],-1)&covered
  assert not mask[white|glass,:3].any(),str(stem)+' protected glass/plate'
  near=depth[...,0].astype(int)*256+depth[...,1];far=back[...,1].astype(int)*256+back[...,2]
  assert np.all(far[covered]<=near[covered]),stem
  assert emit[...,0 if label.startswith('front') else 1].max()>16,stem
  assert emit[...,2].max()>16,stem
assert rim_count==54
print('PASS R34 assets: 576 checksums/canvases, 30 body poses, 54 rim masks, protected glass/plates, paint/hood, front/back depth and lights')
