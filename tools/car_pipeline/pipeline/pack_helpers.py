import numpy as np, json, base64, io
from collections import Counter
from PIL import Image
SS=4; N=128
def b64(a):
    b=io.BytesIO(); Image.fromarray(a).save(b,"PNG",optimize=True); return "data:image/png;base64,"+base64.b64encode(b.getvalue()).decode()
def rd(v,s,f,k): raise RuntimeError("The packer must supply its render-map reader")
def blocks(a): # (512,512[,c]) -> (128,128,16[,c])
    sh=a.shape; r=a.reshape(N,SS,N,SS,*sh[2:]); r=np.moveaxis(r,1,2).reshape(N,N,SS*SS,*sh[2:]); return r
def mode_colour(rgb,m):  # rgb (128,128,16,3) uint8, m (128,128,16) bool
    out=np.zeros((N,N,3),np.uint8)
    key=(rgb[...,0].astype(np.int32)<<16)|(rgb[...,1].astype(np.int32)<<8)|rgb[...,2].astype(np.int32)
    for y in range(N):
        for x in range(N):
            mm=m[y,x]
            if not mm.any(): continue
            c=Counter(key[y,x][mm].tolist()).most_common(1)[0][0]
            out[y,x]=((c>>16)&255,(c>>8)&255,c&255)
    return out
def frame(v,s,f):
    alb=rd(v,s,f,"albedo"); a_hi=alb[...,3]>0
    d=rd(v,s,f,"depth").astype(np.float32)/65535; far=rd(v,s,f,"far")[:,::-1].astype(np.float32)/65535
    nx=rd(v,s,f,"nx").astype(np.float32)/65535*2-1; ny=rd(v,s,f,"ny").astype(np.float32)/65535*2-1
    sp=rd(v,s,f,"spec").astype(np.float32)/255; em=rd(v,s,f,"emit")[...,:3].astype(np.float32)/255
    nz=np.sqrt(np.clip(1-nx*nx-ny*ny,0,1))
    m=blocks(a_hi); cov=m.mean(-1); A=cov>=0.5
    w=m.astype(np.float32); ws=np.maximum(w.sum(-1),1)
    def avg(q): return (blocks(q)*w).sum(-1)/ws
    D=avg(d); F=avg(far); SP=avg(sp)
    vx=avg(nx); vy=avg(ny); vz=avg(nz); L=np.sqrt(vx*vx+vy*vy+vz*vz)+1e-6; vx/=L; vy/=L
    EM=np.stack([np.clip(((blocks(em[...,c])>0.5)*w).sum(-1)/ws*3,0,1) for c in range(3)],-1)
    ALB=mode_colour(blocks(alb[...,:3]),m)
    F=np.minimum(F,D)  # far surface can never be nearer than the front surface
    return dict(a=A,alb=ALB.astype(np.float32),nx=vx,ny=vy,d=D,f=F,sp=SP,em=EM)

def clean(alb,a,em):
    """alb float (N,N,3), a bool, em (N,N,3). returns cleaned alb, silhouette-outlined"""
    A=np.clip(alb+0.5,0,255).astype(np.int32); prot=(em.max(-1)>0.05)|((A[...,0]>190)&(A[...,1]>140)&(A[...,2]<110))
    # 2. merge near-identical palette colours into the more frequent one
    key=(A[...,0]<<16)|(A[...,1]<<8)|A[...,2]; cnt=Counter(key[a&~prot].tolist())
    pal=[c for c,_ in cnt.most_common()]; rep={}; keep=[]
    for c in pal:
        rgb=np.array([(c>>16)&255,(c>>8)&255,c&255])
        for k in keep:
            if np.abs(rgb-np.array([(k>>16)&255,(k>>8)&255,k&255])).sum()<=60: rep[c]=k;break
        else: keep.append(c); rep[c]=c
    for y in range(N):
        for x in range(N):
            if a[y,x] and not prot[y,x]:
                c=rep[int(key[y,x])]; A[y,x]=((c>>16)&255,(c>>8)&255,c&255)
    key=(A[...,0]<<16)|(A[...,1]<<8)|A[...,2]
    # 1. isolated/ragged pixel removal (2 passes)
    for _ in range(2):
        new=A.copy()
        for y in range(1,N-1):
            for x in range(1,N-1):
                if not a[y,x] or prot[y,x]: continue
                nb=[(y+dy,x+dx) for dy in(-1,0,1) for dx in(-1,0,1) if (dy or dx) and a[y+dy,x+dx]]
                if len(nb)<5: continue
                cs=Counter(int(key[p]) for p in nb); same=cs.get(int(key[y,x]),0)
                top,tn=cs.most_common(1)[0]
                if same<=1 and tn>=3 or (same<=2 and tn>=5 and top!=key[y,x]):
                    new[y,x]=((top>>16)&255,(top>>8)&255,top&255)
        A=new; key=(A[...,0]<<16)|(A[...,1]<<8)|A[...,2]
    out=A.astype(np.float32)
    # 5. inner silhouette outline (darken edge pixels)
    ap=np.pad(a,1); edge=a&~(ap[:-2,1:-1]&ap[2:,1:-1]&ap[1:-1,:-2]&ap[1:-1,2:])
    out[edge&~prot]*=0.62
    return out
def enc16(x): return np.clip(np.round(x*65535),0,65535).astype(np.uint16)
