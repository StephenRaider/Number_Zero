import numpy as np, json, base64, io
from collections import Counter
from PIL import Image
P="/tmp/work/out_v3/iso2to1_"; SS=4; N=128
def b64(a):
    b=io.BytesIO(); Image.fromarray(a).save(b,"PNG",optimize=True); return "data:image/png;base64,"+base64.b64encode(b.getvalue()).decode()
def rd(v,s,f,k): return np.array(Image.open(f"{P}{v}_{s}_128_f{f}_{k}.png"))
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
data={}; total=0
for v in ("front","back"):
    for s in ("down","up"):
        fr=[frame(v,s,f) for f in range(8)]
        al=np.stack([x["alb"] for x in fr]); vary=(al.max(0)!=al.min(0)).any(-1)
        keep=vary.copy()
        for _ in range(4):
            p=np.pad(keep,1); keep=keep|p[:-2,1:-1]|p[2:,1:-1]|p[1:-1,:-2]|p[1:-1,2:]|p[:-2,:-2]|p[2:,2:]|p[:-2,2:]|p[2:,:-2]
        for x in fr:
            x["alb"]=np.where(keep[...,None],x["alb"],clean(x["alb"],x["a"],x["em"]))
            x["alb"]=np.where(keep[...,None],x["alb"],x["alb"])
        any_a=np.any([x["a"] for x in fr],axis=0)
        ws=np.maximum(np.sum([x["a"] for x in fr],axis=0).astype(np.float32),1)
        def mean(k):
            acc=sum(x[k]*(x["a"][...,None] if x[k].ndim==3 else x["a"]) for x in fr)
            return acc/(ws[...,None] if fr[0][k].ndim==3 else ws)
        bl=dict(a=any_a,alb=mean("alb"),nx=mean("nx"),ny=mean("ny"),d=mean("d"),f=mean("f"),sp=mean("sp"),em=fr[0]["em"])
        bl['alb']=mean('alb') if False else bl['alb']; fr.append(bl); out=[]
        for x in fr:
            a=x["a"]
            alb=np.zeros((N,N,4),np.uint8); alb[...,:3]=np.clip(x["alb"]+0.5,0,255); alb[...,3]=np.where(a,255,0); alb[~a]=0
            nxe=enc16((x["nx"]+1)/2); nye=enc16((x["ny"]+1)/2)
            nr=np.zeros((N,N,4),np.uint8); nr[...,0]=nxe>>8; nr[...,1]=nxe&255; nr[...,2]=nye>>8; nr[...,3]=np.where(a,255,0); nr[~a]=0
            de=enc16(x["d"]); fe=enc16(x["f"])
            pk=np.zeros((N,N,4),np.uint8); pk[...,0]=de>>8; pk[...,1]=de&255; pk[...,2]=np.clip(x["sp"]*255+0.5,0,255); pk[...,3]=np.where(a,255,0); pk[~a]=0
            p2=np.zeros((N,N,4),np.uint8); p2[...,0]=nye&255; p2[...,1]=fe>>8; p2[...,2]=fe&255; p2[...,3]=np.where(a,255,0); p2[~a]=0
            em=np.zeros((N,N,4),np.uint8); em[...,:3]=np.clip(x["em"]*255+0.5,0,255); em[...,3]=np.where(a,255,0); em[~a]=0
            e=dict(albedo=b64(alb),normal=b64(nr),pack=b64(pk),pack2=b64(p2),emit=b64(em)); total+=sum(len(t) for t in e.values()); out.append(e)
        data[f"{v}_{s}"]=out; print(v,s,"done",flush=True)
meta=json.load(open("/tmp/work/out/iso2to1_meta.json"))
json.dump({"data":data,"meta":meta},open("/tmp/work/assets4.json","w"))
print(round(total/1e6,2),"MB b64")
