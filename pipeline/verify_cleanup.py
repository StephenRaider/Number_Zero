import sys,json,base64,io
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'.runtime'))
import numpy as np
from PIL import Image,ImageDraw
def image(url): return Image.open(io.BytesIO(base64.b64decode(url.split(',')[1])))
sheet=Image.new('RGB',(768,1024),(30,34,44));draw=ImageDraw.Draw(sheet)
for ci,car in enumerate(('silvia','civic')):
    folder=ROOT/'output'/car
    before=json.loads((folder/'assets.before.json').read_text())
    after=json.loads((folder/'assets.improved.json').read_text())
    for vi,view in enumerate(('front_down','back_down')):
        assert len(before['data'][view])==len(after['data'][view])==9
        normal_changes=0;colour_changes=0
        for fi,(a,b) in enumerate(zip(before['data'][view],after['data'][view])):
            aa=np.array(image(a['albedo'])); ab=np.array(image(b['albedo']))
            assert np.array_equal(aa[:,:,3],ab[:,:,3]),'Silhouette changed'
            assert a['emit']==b['emit'],'Lamp/indicator emission changed'
            assert set(np.unique(ab[:,:,3]))<=set((0,255))
            assert np.count_nonzero((ab[:,:,:3].min(-1)==255)&(ab[:,:,3]>0))>=3,'White plates lost'
            assert np.count_nonzero((np.abs(ab[:,:,:3].astype(int)-[66,69,75]).max(-1)<6)&(ab[:,:,3]>0))>40,'Grey windows lost'
            colour_changes+=np.any(aa!=ab,axis=-1).sum()
            normal_changes+=np.any(np.array(image(a['normal']))!=np.array(image(b['normal'])),axis=-1).sum()
            n=np.array(image(b['normal'])).astype(float);q=np.array(image(b['pack2'])).astype(float)
            nx=(n[:,:,0]*256+n[:,:,1])/65535*2-1;ny=(n[:,:,2]*256+q[:,:,0])/65535*2-1
            assert np.max((nx*nx+ny*ny)[ab[:,:,3]>0])<1.001,'Invalid normal'
        assert colour_changes>0 and normal_changes>0,'Comparison is identical'
        row=ci*2+vi
        for col,data in enumerate((before,after)):
            im=image(data['data'][view][0]['albedo']).resize((384,384),Image.Resampling.NEAREST).crop((0,70,384,326))
            sheet.paste(im,(384*col,256*row),im)
            draw.text((384*col+12,256*row+8),f'{car.upper()} {view.split("_")[0]} / '+('BEFORE' if col==0 else 'AFTER'),fill=(235,235,235))
        print('PASS',car,view,'colour changes',int(colour_changes),'normal changes',int(normal_changes),'silhouette and lamps preserved')
sheet.save(ROOT/'output/cleanup-comparison.png')
