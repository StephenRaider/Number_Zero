"""Blender 3.6/5.2 background renderer. Geometry is in metres, front=-Y."""
import bpy, sys, math, json
import numpy as np
from pathlib import Path
from mathutils import Vector, Euler, Matrix

ROOT=Path(__file__).resolve().parents[1]
args=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []
KEY=args[0] if args else 'silvia'
NFR=int(args[1]) if len(args)>1 else 8
MATERIAL_ONLY='--materials-only' in args
DETAILED_LAMPS='--detailed-lamps' in args
EMISSION_ONLY='--emission-only' in args
OUT=ROOT/'output'/(KEY+'-detailed' if DETAILED_LAMPS else KEY)
OUT.mkdir(parents=True,exist_ok=True)
SRC=ROOT/('assets/silvia/silvia.glb' if KEY=='silvia' else 'assets/civic/source/2000 Honda Civic Type-R (EK9).glb')
N=128; SS=4; SCALE=26.; T=Vector((0,0,0)); WRANGE=2.6
VIEWS={'front':-45.,'back':135.}
GLASS=(.055,.060,.070)
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=str(SRC))
sc=bpy.context.scene
meshes=[o for o in sc.objects if o.type=='MESH']
# Bake imported hierarchies so spinning and dimensional normalization are deterministic.
for o in meshes:
    matrix=o.matrix_world.copy(); o.parent=None; o.matrix_world=Matrix.Identity(4)
    o.data.transform(matrix)
ps=[v.co for o in meshes for v in o.data.vertices]
lo=Vector([min(p[i] for p in ps) for i in range(3)])
hi=Vector([max(p[i] for p in ps) for i in range(3)])
factor=(4.18/(hi.y-lo.y)) if KEY=='civic' else 1.
offset=Vector(((lo.x+hi.x)/2,(lo.y+hi.y)/2,lo.z))
for o in meshes:
    for v in o.data.vertices: v.co=(v.co-offset)*factor

def material(name,rgb):
    m=bpy.data.materials.new(name); m.diffuse_color=(*rgb,1); m.use_nodes=True
    b=m.node_tree.nodes.get('Principled BSDF'); b.inputs['Base Color'].default_value=(*rgb,1)
    return m
white=material('Plain white license plate',(1,1,1))
glass=material('Opaque dark grey window',GLASS)
# The Civic includes opaque backing polygons coincident with glass; tint those too.
for o in meshes:
    if KEY=='civic' and ('Glass' in o.name) and any(s.material and ('WindowFront' in s.material.name or 'PlasticRoughBlack' in s.material.name) for s in o.material_slots):
        o.data.materials.clear(); o.data.materials.append(glass)
    for slot in o.material_slots:
        if slot.material and (slot.material.name=='body_win' or 'Glass_WindowFront' in slot.material.name): slot.material=glass
        if slot.material and slot.material.name in ('number_pl','number_tex'): slot.material=white
    if KEY=='civic' and 'Boot6_' in o.name and 'PlasticRoughBlack' in o.name:
        o.data.materials.clear(); o.data.materials.append(white)

if KEY=='civic':
    # Original has no dedicated plate mesh. New opaque faces cover the two recesses.
    for side,y,z in [('front',-2.109,.39)]:
        bpy.ops.mesh.primitive_cube_add(size=1,location=(0,y,z))
        o=bpy.context.object; o.name='Plain white '+side+' plate'; o.dimensions=(.46,.018,.17)
        bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
        o.data.materials.append(white)
        o.data.transform(o.matrix_world); o.matrix_world=Matrix.Identity(4); meshes.append(o)

def base_color(m):
    b=m.node_tree.nodes.get('Principled BSDF') if m.use_nodes else None
    return tuple(b.inputs['Base Color'].default_value)[:3] if b else tuple(m.diffuse_color)[:3]
COL={m.name:base_color(m) for m in bpy.data.materials}
TEX={}
for m in bpy.data.materials:
    if m.use_nodes:
        b=m.node_tree.nodes.get('Principled BSDF')
        if b and b.inputs['Base Color'].is_linked:
            node=b.inputs['Base Color'].links[0].from_node
            if node.type=='TEX_IMAGE': TEX[m.name]=node.image
SPEC={}; EM={}
lamp_atlas_mask=None
if DETAILED_LAMPS and KEY=='civic':
    atlas=TEX['hM_LightBucket_Max1']; width,height=atlas.size
    pixels=np.empty(width*height*4,dtype=np.float32);atlas.pixels.foreach_get(pixels)
    pixels=pixels.reshape(height,width,4);r,g,b=pixels[...,:3].transpose(2,0,1)
    amber=(r>g*1.35)&(g>b*1.4)&(r>.15)
    lum=.2126*r+.7152*g+.0722*b
    mask=np.zeros_like(pixels);mask[...,0]=np.where(amber,0,.025+.11*lum**3)
    mask[...,2]=amber;mask[...,3]=1
    lamp_atlas_mask=bpy.data.images.new('Civic reflector and amber indicator emission',width=width,height=height,alpha=True)
    lamp_atlas_mask.colorspace_settings.name='Non-Color';lamp_atlas_mask.use_fake_user=True
    lamp_atlas_mask.pixels.foreach_set(mask.ravel());lamp_atlas_mask.pack()
# The S13 rear clusters share a textured material, rather than having a
# separately named amber material. Classify their atlas into independent tail
# and indicator channels. Image pixel rows run from bottom to top in Blender.
silvia_lamp_mask=None
if KEY=='silvia':
    atlas=TEX['Material.004']
    width,height=atlas.size
    pixels=np.empty(width*height*4,dtype=np.float32)
    atlas.pixels.foreach_get(pixels)
    pixels=pixels.reshape(height,width,4)
    yy,xx=np.mgrid[0:height,0:width]
    u=(xx+.5)/width; v=(yy+.5)/height
    r,g,b=pixels[...,:3].transpose(2,0,1)
    coloured=(r>g*1.35)&(r>b*1.5)&(r>.08)
    amber=coloured&(u>.34)&(v<.50)&(v>.16)
    tail=coloured&(v>=.50)&(v<.84)
    mask=np.zeros((height,width,4),dtype=np.float32)
    mask[...,1]=tail; mask[...,2]=amber; mask[...,3]=1
    assert amber.sum()>1000 and tail.sum()>1000, 'Rear lamp atlas classification failed'
    silvia_lamp_mask=bpy.data.images.new('Silvia rear tail and amber indicator mask',width=width,height=height,alpha=True)
    silvia_lamp_mask.colorspace_settings.name='Non-Color'
    silvia_lamp_mask.use_fake_user=True
    silvia_lamp_mask.pixels.foreach_set(mask.ravel())
    silvia_lamp_mask.pack()
    print('SILVIA_REAR_MASK',int(tail.sum()),'tail texels',int(amber.sum()),'amber texels',flush=True)
for m in bpy.data.materials:
    name=m.name.lower()
    SPEC[m.name]=1. if m==glass else .65 if any(k in name for k in ('paint','chrome','rim','mirror')) else .12
    if m==white: SPEC[m.name]=.25
    # Raise pure-black rubber slightly; windows use the explicit grey above.
    if max(COL[m.name])<.001: COL[m.name]=(.012,.014,.018)
    if 'lightglassnormal_clear' in name or 'headlight_front_glass' in name: COL[m.name]=(.62,.67,.7)
    if 'lightglassnormal_red' in name: COL[m.name]=(.55,.015,.02)
    if 'wheel1a' in name: COL[m.name]=(.018,.02,.024); TEX.pop(m.name,None)
    if any(k in name for k in ('interior','engine')) or ('lightbucket' in name and not DETAILED_LAMPS): TEX.pop(m.name,None)
    if DETAILED_LAMPS and any(k in name for k in ('lightglassnormal_clear','headlight_front_glass')): SPEC[m.name]=.85
for o in meshes:
    for slot in o.material_slots:
        if not slot.material: continue
        name=slot.material.name.lower()
        ch=(0,0,0)
        if 'headlight_front_glass' in name or ('lightglassnormal_clear' in name and 'HeadLight' in o.name): ch=(1,0,0)
        elif 'lightglassnormal_red' in name: ch=(0,1,0)
        elif 'blinker' in name or ('lightglass' in name and 'orange' in name): ch=(0,0,1)
        # Silvia's textured rear clusters are classified with their atlas below.
        EM[(o.name,slot.material.name)]=ch

sc.render.engine='CYCLES'; sc.cycles.device='CPU'; sc.cycles.samples=1
sc.cycles.use_denoising=False; sc.cycles.use_adaptive_sampling=False; sc.cycles.filter_width=.01
sc.render.film_transparent=True; sc.render.resolution_percentage=100
sc.render.resolution_x=N*SS; sc.render.resolution_y=N*SS
sc.view_settings.view_transform='Standard'; sc.view_settings.look='None'
sc.view_settings.exposure=0; sc.view_settings.gamma=1
sc.render.image_settings.file_format='PNG'
cam_data=bpy.data.cameras.new('Isometric camera'); cam_data.type='ORTHO'; cam_data.ortho_scale=N/SCALE
cam_data.clip_start=.1; cam_data.clip_end=100; cam_data.shift_y=15/N
cam=bpy.data.objects.new('Isometric camera',cam_data); sc.collection.objects.link(cam); sc.camera=cam

def emission(m,rgb,image=None):
    m.use_nodes=True; nt=m.node_tree; nt.nodes.clear()
    e=nt.nodes.new('ShaderNodeEmission'); e.inputs['Color'].default_value=(*rgb,1)
    if image:
        tex=nt.nodes.new('ShaderNodeTexImage'); tex.image=image; nt.links.new(tex.outputs['Color'],e.inputs['Color'])
    out=nt.nodes.new('ShaderNodeOutputMaterial'); nt.links.new(e.outputs[0],out.inputs['Surface'])
    if hasattr(m,'blend_method'): m.blend_method='OPAQUE'

def clear_lens(m):
    return DETAILED_LAMPS and ('LightGlassNormal_Clear' in m.name or 'headlight_front_glass' in m.name)

def transparent(m):
    nt=m.node_tree;nt.nodes.clear()
    tr=nt.nodes.new('ShaderNodeBsdfTransparent');out=nt.nodes.new('ShaderNodeOutputMaterial')
    nt.links.new(tr.outputs[0],out.inputs['Surface'])

def dot(nt,a,v):
    n=nt.nodes.new('ShaderNodeVectorMath'); n.operation='DOT_PRODUCT'; nt.links.new(a,n.inputs[0]); n.inputs[1].default_value=v
    return n.outputs['Value']
def madd(nt,x,mul,add):
    n=nt.nodes.new('ShaderNodeMath'); n.operation='MULTIPLY_ADD'; nt.links.new(x,n.inputs[0]); n.inputs[1].default_value=mul; n.inputs[2].default_value=add
    return n.outputs[0]
def override(kind,R,U,B):
    m=material('ovr_'+kind,(0,0,0)); nt=m.node_tree; nt.nodes.clear()
    g=nt.nodes.new('ShaderNodeNewGeometry'); e=nt.nodes.new('ShaderNodeEmission'); out=nt.nodes.new('ShaderNodeOutputMaterial')
    nt.links.new(e.outputs[0],out.inputs['Surface'])
    if kind in ('nx','ny'): val=madd(nt,dot(nt,g.outputs['Normal'],R if kind=='nx' else U),.5,.5)
    else: val=madd(nt,dot(nt,g.outputs['Position'],B),1/(2*WRANGE),.5)
    c=nt.nodes.new('ShaderNodeCombineXYZ')
    for i in range(3): nt.links.new(val,c.inputs[i])
    nt.links.new(c.outputs[0],e.inputs['Color'])
    return m

# Spin each wheel about its own axle, even for glTFs merging all four wheels.
wheel_objects=[o for o in meshes if any(s.material and any(k in s.material.name.lower() for k in ('wheel_','rim','wheel1a')) for s in o.material_slots)]
groups={}
for o in wheel_objects:
    for v in o.data.vertices:
        key=(v.co.x>0,v.co.y>0); groups.setdefault(key,[]).append(v.co.copy())
centers={key:Vector([(min(p[i] for p in ps)+max(p[i] for p in ps))/2 for i in range(3)]) for key,ps in groups.items()}
wheel_base={o.name:[v.co.copy() for v in o.data.vertices] for o in wheel_objects}
def spin(deg):
    rot=Matrix.Rotation(math.radians(deg),3,'X')
    for o in wheel_objects:
        for v,p in zip(o.data.vertices,wheel_base[o.name]):
            c=centers[(p.x>0,p.y>0)]; v.co=c+rot@(p-c)
        o.data.update()

material_ids={m.name:i+1 for i,m in enumerate(sorted(bpy.data.materials,key=lambda m:m.name))}
(OUT/'material_ids.json').write_text(json.dumps(material_ids,indent=2))
for m in bpy.data.materials:
    emission(m,COL.get(m.name,(.5,.5,.5)),TEX.get(m.name))
    if clear_lens(m): transparent(m)
if not MATERIAL_ONLY and not EMISSION_ONLY: bpy.ops.wm.save_as_mainfile(filepath=str(OUT/(KEY+'_prepared.blend')))
meta={'elevation_deg':30.,'azimuth_front':-45.,'azimuth_back':135.,'anchor_world':[0,0,0], 'depth':{'range_m':2*WRANGE}, 'sizes':{'128':{'canvas':128,'px_per_metre':SCALE,'tile_axis_px':[round(SCALE/math.sqrt(2),4),round(SCALE/math.sqrt(2)/2,4)],'anchor_px':[64.,79.]}}, 'car':KEY,'frames':NFR,'window_srgb':'#42454b','plates':'plain white','length_m':round((hi.y-lo.y)*factor,3),'detailed_lamps':DETAILED_LAMPS,'source_model':str(SRC)}
if not MATERIAL_ONLY and not EMISSION_ONLY: (OUT/'meta.json').write_text(json.dumps(meta,indent=2))
for view,az in VIEWS.items():
    M=Euler((math.radians(60),0,math.radians(az))).to_matrix()
    R,U,B=[M@Vector(v) for v in ((1,0,0),(0,1,0),(0,0,1))]
    for frame in range(NFR):
        spin(frame*45)
        for kind in (('emit',) if EMISSION_ONLY else ('material',) if MATERIAL_ONLY else ('albedo','nx','ny','depth','far','spec','emit','material')):
            cam.rotation_euler=M.to_euler(); cam.location=B*30
            if kind=='far': cam.rotation_euler=(M@Matrix.Rotation(math.pi,3,'Y')).to_euler(); cam.location=-B*30
            sc.view_settings.view_transform='Standard' if kind=='albedo' or (kind=='emit' and not DETAILED_LAMPS) else 'Raw'
            sc.render.image_settings.color_mode='RGBA' if kind in ('albedo','emit') else 'BW'
            sc.render.image_settings.color_depth='16' if kind in ('nx','ny','depth','far') else '8'
            vl=bpy.context.view_layer; vl.material_override=None
            temporary=[]
            if kind=='material':
                for m in list(bpy.data.materials):
                    if m.name in material_ids: emission(m,(material_ids[m.name]/255,)*3)
            elif kind in ('albedo','spec'):
                for m in list(bpy.data.materials):
                    if m.name.startswith('ovr_'): continue
                    rgb=COL.get(m.name,(.5,.5,.5)) if kind=='albedo' else (SPEC.get(m.name,.2),)*3
                    emission(m,rgb,TEX.get(m.name) if kind=='albedo' else None)
                    if kind=='albedo' and clear_lens(m): transparent(m)
                    if kind=='albedo' and DETAILED_LAMPS and m.name=='headlight_chrome':
                        nt=m.node_tree;ao=nt.nodes.new('ShaderNodeAmbientOcclusion');ao.inputs['Distance'].default_value=.18
                        ao.inputs['Color'].default_value=(.75,.78,.80,1);ao.samples=8
                        value=madd(nt,ao.outputs['AO'],.55,.45)
                        colour=nt.nodes.new('ShaderNodeCombineXYZ')
                        for axis in ('X','Y','Z'): nt.links.new(value,colour.inputs[axis])
                        nt.links.new(colour.outputs[0],nt.nodes.get('Emission').inputs['Color'])
            elif kind=='emit':
                for o in meshes:
                    for slot in o.material_slots:
                        if not slot.material: continue
                        old=slot.material; ch=EM.get((o.name,old.name),(0,0,0))
                        tmp=material('Emission temporary',ch); emission(tmp,ch)
                        if clear_lens(old): transparent(tmp)
                        if DETAILED_LAMPS and KEY=='civic' and old.name=='hM_LightBucket_Max1' and 'HeadLight' in o.name:
                            emission(tmp,(0,0,0),lamp_atlas_mask)
                        if DETAILED_LAMPS and KEY=='silvia' and old.name=='headlight_chrome':
                            nt=tmp.node_tree;geo=nt.nodes.new('ShaderNodeNewGeometry');sep=nt.nodes.new('ShaderNodeSeparateXYZ');nt.links.new(geo.outputs['Position'],sep.inputs[0])
                            gate=nt.nodes.new('ShaderNodeMath');gate.operation='LESS_THAN';nt.links.new(sep.outputs['Y'],gate.inputs[0]);gate.inputs[1].default_value=-1.7
                            value=madd(nt,gate.outputs[0],.11,0)
                            combine=nt.nodes.new('ShaderNodeCombineXYZ');nt.links.new(value,combine.inputs['X']);nt.links.new(combine.outputs[0],nt.nodes.get('Emission').inputs['Color'])
                        if KEY=='silvia' and old.name=='Material.004':
                            nt=tmp.node_tree; g=nt.nodes.new('ShaderNodeNewGeometry'); sep=nt.nodes.new('ShaderNodeSeparateXYZ'); nt.links.new(g.outputs['Position'],sep.inputs[0])
                            test=nt.nodes.new('ShaderNodeMath'); test.operation='GREATER_THAN'; nt.links.new(sep.outputs['Y'],test.inputs[0]); test.inputs[1].default_value=1.8
                            gate=nt.nodes.new('ShaderNodeCombineXYZ')
                            for axis in ('X','Y','Z'): nt.links.new(test.outputs[0],gate.inputs[axis])
                            tex=nt.nodes.new('ShaderNodeTexImage'); tex.image=silvia_lamp_mask; tex.interpolation='Closest'
                            masked=nt.nodes.new('ShaderNodeVectorMath'); masked.operation='MULTIPLY'
                            nt.links.new(tex.outputs['Color'],masked.inputs[0]); nt.links.new(gate.outputs[0],masked.inputs[1])
                            nt.links.new(masked.outputs[0],nt.nodes.get('Emission').inputs['Color'])
                        slot.material=tmp; temporary.append((slot,old,tmp))
            else: vl.material_override=override(kind,R,U,B)
            bpy.context.view_layer.update()
            sc.render.filepath=str(OUT/f'{view}_f{frame}_{kind}.png')
            bpy.ops.render.render(write_still=True)
            vl.material_override=None
            for slot,old,tmp in temporary: slot.material=old; bpy.data.materials.remove(tmp)
            print('PROGRESS',KEY,view,frame,kind,flush=True)
print('DONE',KEY,flush=True)
