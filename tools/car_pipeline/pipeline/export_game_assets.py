"""Publish only runtime-ready car maps into a separate Number_Zero checkout.

Usage: packing-python pipeline/export_game_assets.py --target <game-repository>
Blender sources, render passes, labs and diagnostics remain in this workspace.
"""
import argparse,hashlib,json,shutil,struct
from pathlib import Path

import sys as _path_sys; _path_sys.path.insert(0,str(Path(__file__).resolve().parent))
from settings import WORKSPACE as ROOT, CODE, GAME, SOURCES
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--target',required=True,type=Path)
parser.add_argument('--car',choices=['silvia','civic','testarossa','supra'])
parser.add_argument('--include-pitch',action='store_true',help='Promote the four packed body pitch samples for the selected car too.')
parser.add_argument('--traffic',action='store_true',help='Promote reviewed traffic-runtime exports only.')
args=parser.parse_args();target=args.target.resolve()
if target==ROOT or ROOT in target.parents:
    raise SystemExit('Target must be the separate game checkout, not the experiment workspace.')
if not (target/'.git').exists():raise SystemExit('Target must be an existing Git checkout.')
if args.traffic:
    source=ROOT/'output/traffic-runtime';catalog=json.loads((source/'catalog.json').read_text());count=0
    for item in catalog['cars']:
        folder=source/item['id'];dest=target/'assets/traffic'/item['id'];meta=json.loads((folder/'manifest.json').read_text())
        previous=json.loads((dest/'manifest.json').read_text()) if (dest/'manifest.json').exists() else {}
        dest.mkdir(parents=True,exist_ok=True)
        for name,digest in meta['files'].items():
            src=folder/name;dst=dest/name;raw=src.read_bytes()
            assert hashlib.sha256(raw).hexdigest()==digest and raw[:8]==b'\x89PNG\r\n\x1a\n' and struct.unpack('>II',raw[16:24])==(256,256)
            if dst.exists() and dst.read_bytes()!=raw:assert hashlib.sha256(dst.read_bytes()).hexdigest()==previous.get('files',{}).get(name),'Modified game-side traffic asset'
            shutil.copyfile(src,dst);count+=1
        shutil.copyfile(folder/'manifest.json',dest/'manifest.json')
    shutil.copyfile(source/'catalog.json',target/'assets/traffic/catalog.json')
    print('EXPORTED TRAFFIC',len(catalog['cars']),'models,',count,'verified PNGs; player cars unchanged')
    raise SystemExit(0)
KINDS=['albedo','normal','pack','pack2','emit','mask']
CARS=[('silvia','nissan_silvia_s13','Nissan Silvia S13'),('civic','honda_civic_ek9','Honda Civic Type R EK9'),('testarossa','ferrari_testarossa','Ferrari Testarossa'),('supra','toyota_supra_a80_sz','Toyota Supra A80 SZ')]
catalog=json.loads((target/'assets/cars/catalog.json').read_text()) if args.car and (target/'assets/cars/catalog.json').exists() else {'schema_version':1,'cars':[]}
if args.car:CARS=[c for c in CARS if c[0]==args.car];catalog['cars']=[c for c in catalog['cars'] if c['id'] not in [v[1] for v in CARS]]
total_files=0;total_bytes=0
for source_key,car_id,title in CARS:
    source=ROOT/'output'/(source_key+'-modular');dest=target/'assets/cars'/car_id
    if not (source/'assets.json').exists():raise SystemExit(f'Rendered assets missing: {source}')
    dataset=json.loads((source/'assets.json').read_text())
    previous=json.loads((dest/'manifest.json').read_text()) if (dest/'manifest.json').exists() else {}
    checksums={}
    def publish(relative,render_source=None,render_relative=None):
        global total_files,total_bytes
        src=(render_source or source)/(render_relative or relative);dst=dest/relative;raw=src.read_bytes()
        if raw[:8]!=b'\x89PNG\r\n\x1a\n' or struct.unpack('>II',raw[16:24])!=(128,128):
            raise ValueError(f'Expected a packed 128 x 128 runtime PNG: {src}')
        if dst.exists() and dst.read_bytes()!=raw:
            recorded=previous.get('files',{}).get(relative)
            if recorded is None or hashlib.sha256(dst.read_bytes()).hexdigest()!=recorded:
                raise ValueError(f'Game-side asset was modified; refusing to overwrite: {dst}')
        dst.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(src,dst)
        checksums[relative]=hashlib.sha256(raw).hexdigest();total_files+=1;total_bytes+=len(raw)
        return relative
    poses={}
    for label in sorted(dataset['data']):
        body={kind:publish(f'sprites/{label}_body_{kind}.png') for kind in KINDS}
        frames=[{kind:publish(f'sprites/{label}_wheels_f{fi}_{kind}.png') for kind in KINDS} for fi in range(len(dataset['animation'][label]))]
        masks={name:publish(f'masks/{label}_{name}.png') for name in ['paint-primary','paint-secondary','hood']}
        poses[label]={'body':body,'paint_masks':masks,'stock_wheels':{'frames':frames,'spin_frames':8,'blur_frame':8}}
    meta=dict(dataset['meta']);meta.pop('source_model',None)
    dest.mkdir(parents=True,exist_ok=True)
    for name,data in [('meta.json',meta),('wheels.json',dataset['wheels'])]:
        (dest/name).write_text(json.dumps(data,indent=2)+'\n',encoding='utf-8')
    body_pitch={}
    if args.include_pitch:
        for label in poses:
            body_pitch[label]={}
            for sample in ['m2','m1','p1','p2']:
                origin=ROOT/'output'/f'{source_key}-modular-pitch_{sample}'
                body_pitch[label][sample]={kind:publish(f'sprites/{label}_body_pitch_{sample}_{kind}.png',origin,f'sprites/{label}_body_{kind}.png') for kind in KINDS}
    manifest={'schema_version':1,'id':car_id,'name':title,'canvas_px':[128,128],
              'ride_height_px':[0,1,2],'meta':'meta.json','wheel_placement':'wheels.json',
              'wheel_asset_status':'Prototype stock wheels, currently rendered per car; shared standard-size wheel library is planned.',
              'poses':poses,'files':dict(sorted(checksums.items()))}
    if args.include_pitch:manifest['body_pitch']={'angles_deg':[-3,-1.5,0,1.5,3],'pivot_m':[0,0,.45],'samples':body_pitch,'wheels':'Reuse existing independent wheel frames.'}
    (dest/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
    catalog['cars'].append({'id':car_id,'name':title,'manifest':f'{car_id}/manifest.json'})
    print('EXPORTED',car_id,len(checksums),'runtime textures',flush=True)
(target/'assets/cars/catalog.json').write_text(json.dumps(catalog,indent=2)+'\n',encoding='utf-8')
print('GAME ASSETS',total_files,'PNGs,',total_bytes,'bytes',flush=True)
