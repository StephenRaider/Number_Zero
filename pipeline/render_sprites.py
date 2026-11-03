import bpy, math, json, sys
from mathutils import Vector, Euler

SRC="/tmp/work/car/source/untitled_fer_old.blend"
OUT="/tmp/work/out"
ELEV=float(sys.argv[sys.argv.index("--elev")+1]) if "--elev" in sys.argv else 30.0   # 30 deg => exact 2:1 pixel-art iso tiles
TAG=sys.argv[sys.argv.index("--tag")+1] if "--tag" in sys.argv else "iso2to1"
T=Vector((0.0,-0.15,0.0))         # ground point under car centre = sprite anchor
WRANGE=1.8                         # depth encoded over w in [-1.8, +1.8] m around anchor
SIZES={128:28, 256:56}             # canvas px -> px that a 1 m grid axis advances horizontally
VIEWS={"front":-45.0, "back":135.0}  # camera azimuth (Blender Z rotation)
STATES={"down":1, "up":60}          # pop-up headlights animation frame

bpy.ops.wm.open_mainfile(filepath=SRC)
sc=bpy.context.scene
sc.render.engine='CYCLES'
sc.cycles.device='CPU'; sc.cycles.samples=1; sc.cycles.use_denoising=False
sc.cycles.filter_width=0.01; sc.cycles.use_adaptive_sampling=False
sc.render.film_transparent=True
sc.render.resolution_percentage=100
sc.display_settings.display_device='sRGB'
sc.view_settings.look='None'; sc.view_settings.exposure=0; sc.view_settings.gamma=1

# ---- tint glass black (permanent in the saved blend, too)
GLASS=bpy.data.materials['Glass']
def base_color(m):
    if m.use_nodes:
        b=m.node_tree.nodes.get("Principled BSDF")
        if b: return tuple(b.inputs["Base Color"].default_value)[:3]
    return tuple(m.diffuse_color)[:3]
if GLASS.use_nodes:
    b=GLASS.node_tree.nodes.get("Principled BSDF")
    if b:
        b.inputs["Base Color"].default_value=(0.004,0.004,0.004,1)
        for k in ("Transmission Weight","Alpha"):
            if k in b.inputs: b.inputs[k].default_value = 0.0 if k=="Transmission Weight" else 1.0
GLASS.diffuse_color=(0.004,0.004,0.004,1)
bpy.ops.wm.save_as_mainfile(filepath=OUT+"/testarossa_glass_black.blend", copy=True)

cols={m.name:base_color(m) for m in bpy.data.materials}
cols['Glass']=(0.004,0.004,0.004)
SPEC={'Glass':1.0,'mirror':0.9,'_metal':0.7,'rims':0.8,'Red':0.55,'Lights':0.8,'Tires':0.05,'Black_main':0.25,
      'Blackest_black':0.1,'White_interior':0.2,'Orange_lights':0.5,'Red_light':0.5,'Yellow':0.3,'Material':0.2}

def new_tree(m):
    m.use_nodes=True
    nt=m.node_tree; nt.nodes.clear()
    return nt
def emission_mat(m, rgb):
    nt=new_tree(m)
    e=nt.nodes.new('ShaderNodeEmission'); e.inputs['Color'].default_value=(*rgb,1); e.inputs['Strength'].default_value=1
    o=nt.nodes.new('ShaderNodeOutputMaterial'); nt.links.new(e.outputs[0],o.inputs['Surface'])
def vec(nt,v):
    n=nt.nodes.new('ShaderNodeCombineXYZ')
    for i,k in enumerate(('X','Y','Z')): n.inputs[k].default_value=v[i]
    return n.outputs[0]
def dot(nt,a,b):
    n=nt.nodes.new('ShaderNodeVectorMath'); n.operation='DOT_PRODUCT'
    nt.links.new(a,n.inputs[0]); nt.links.new(b,n.inputs[1]); return n.outputs['Value']
def madd(nt,x,mul,add):
    n=nt.nodes.new('ShaderNodeMath'); n.operation='MULTIPLY_ADD'
    nt.links.new(x,n.inputs[0]); n.inputs[1].default_value=mul; n.inputs[2].default_value=add; return n.outputs[0]

def build_override(kind, R, up, back):
    m=bpy.data.materials.new("ovr_"+kind); nt=new_tree(m)
    g=nt.nodes.new('ShaderNodeNewGeometry')
    e=nt.nodes.new('ShaderNodeEmission'); o=nt.nodes.new('ShaderNodeOutputMaterial'); nt.links.new(e.outputs[0],o.inputs['Surface'])
    if kind=='normal':
        c=nt.nodes.new('ShaderNodeCombineXYZ')
        for i,ax in enumerate((R,up,back)):
            v=madd(nt,dot(nt,g.outputs['Normal'],vec(nt,ax)),0.5,0.5)
            nt.links.new(v,c.inputs[i])
        nt.links.new(c.outputs[0],e.inputs['Color'])
    else: # depth: w = dot(P - T, back); encoded (w+WRANGE)/(2*WRANGE), 1 = nearest
        sub=nt.nodes.new('ShaderNodeVectorMath'); sub.operation='SUBTRACT'
        nt.links.new(g.outputs['Position'],sub.inputs[0]); sub.inputs[1].default_value=tuple(T)
        w=dot(nt,sub.outputs[0],vec(nt,back))
        d=madd(nt,w,1.0/(2*WRANGE),0.5)
        c=nt.nodes.new('ShaderNodeCombineXYZ')
        for i in range(3): nt.links.new(d,c.inputs[i])
        nt.links.new(c.outputs[0],e.inputs['Color'])
    return m

cam_data=bpy.data.cameras.new("isocam"); cam_data.type='ORTHO'; cam_data.clip_start=0.1; cam_data.clip_end=60
cam=bpy.data.objects.new("isocam",cam_data); sc.collection.objects.link(cam); sc.camera=cam

# vertical centring (same for every view/state so all sprites share one anchor)
mesh_objs=[o for o in bpy.data.objects if o.type=='MESH']
corners=[]
for st,fr in STATES.items():
    sc.frame_set(fr); bpy.context.view_layer.update()
    for o in mesh_objs:
        for c in o.bound_box: corners.append(o.matrix_world@Vector(c))
def axes(az):
    M=Euler((math.radians(90-ELEV),0,math.radians(az)),'XYZ').to_matrix()
    return M, M@Vector((1,0,0)), M@Vector((0,1,0)), M@Vector((0,0,1))
ups=[]
for az in VIEWS.values():
    M,R,U,B=axes(az); ups+= [(c-T).dot(U) for c in corners]
ymid=(max(ups)+min(ups))/2
meta={"elevation_deg":ELEV,"azimuth_front":VIEWS['front'],"azimuth_back":VIEWS['back'],"anchor_world":list(T),
      "depth":{"encoding":"white=nearest; w=(value-0.5)*%.2f metres toward camera relative to anchor"%(2*WRANGE),"range_m":2*WRANGE},
      "sizes":{}}

def render(path, kind, N, s, az, shift_px):
    M,R,U,B=axes(az)
    cam.rotation_euler=Euler((math.radians(90-ELEV),0,math.radians(az)))
    cam.location=T+B*30.0
    cam_data.ortho_scale=N/s
    cam_data.shift_x=0; cam_data.shift_y=shift_px/ N
    sc.render.resolution_x=N; sc.render.resolution_y=N
    vs=sc.view_settings
    vl=bpy.context.view_layer
    vl.material_override=None
    if kind=='albedo':
        vs.view_transform='Standard'
        for m in bpy.data.materials:
            if m.name.startswith("ovr_"): continue
            emission_mat(m, cols.get(m.name,(0.5,0.5,0.5)))
        sc.render.image_settings.color_mode='RGBA'; sc.render.image_settings.color_depth='8'
    elif kind=='spec':
        vs.view_transform='Raw'
        for m in bpy.data.materials:
            if m.name.startswith("ovr_"): continue
            v=SPEC.get(m.name,0.2); emission_mat(m,(v,v,v))
        sc.render.image_settings.color_mode='BW'; sc.render.image_settings.color_depth='8'
    else:
        vs.view_transform='Raw'
        vl.material_override=build_override(kind,tuple(R),tuple(U),tuple(B))
        if kind=='normal': sc.render.image_settings.color_mode='RGBA'; sc.render.image_settings.color_depth='8'
        else: sc.render.image_settings.color_mode='BW'; sc.render.image_settings.color_depth='16'
    sc.render.image_settings.file_format='PNG'
    sc.render.filepath=path
    bpy.ops.render.render(write_still=True)
    vl.material_override=None

for N,tilepx in SIZES.items():
    s=tilepx/math.sqrt(0.5)                 # px per metre along each ground axis
    shift_px=round(ymid*s)                  # whole pixels
    meta["sizes"][str(N)]={"canvas":N,"px_per_metre":s,"tile_axis_px":[tilepx,round(tilepx*math.sin(math.radians(ELEV)))],
                           "anchor_px":[N/2, N/2+shift_px]}
    for vname,az in VIEWS.items():
        for sname,fr in STATES.items():
            sc.frame_set(fr); bpy.context.view_layer.update()
            for kind in ('albedo','normal','depth','spec'):
                render(f"{OUT}/{TAG}_{vname}_{sname}_{N}_{kind}.png",kind,N,s,az,shift_px)
json.dump(meta,open(f"{OUT}/{TAG}_meta.json","w"),indent=1)
print("DONE",meta)
