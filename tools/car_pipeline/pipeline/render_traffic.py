"""Native traffic sprites: four wheel phases x three lane-change poses, no pitch."""
import bpy,sys,math,json
from pathlib import Path
from mathutils import Vector,Matrix,Euler
sys.path.insert(0,str(Path(__file__).resolve().parent))
from traffic_models import MODELS
import sys as _path_sys; _path_sys.path.insert(0,str(Path(__file__).resolve().parent))
from settings import WORKSPACE as ROOT, CODE, GAME, SOURCES;N=256;SS=3;PPM=26.;RANGE=12.
args=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []
heights={'corolla':1.47,'swift':1.5,'crown':1.515,'hiace':1.96,'rosa':2.6}
paint_names={'corolla':{'Super_White_II_040','Silver_Metallic_199'},'swift':{'body_color'},'crown':{'Fix0101Mtl'},'hiace':{'paint'},'rosa':{'PAINT','Material.008','Metallic_10','Metallic_3'}}
flip={'corolla':False,'swift':True,'crown':False,'hiace':False,'rosa':True}

def material(name,rgb):
 m=bpy.data.materials.new(name);m.use_nodes=True;nt=m.node_tree;nt.nodes.clear();e=nt.nodes.new('ShaderNodeEmission');e.inputs['Color'].default_value=(*rgb,1);o=nt.nodes.new('ShaderNodeOutputMaterial');nt.links.new(e.outputs[0],o.inputs[0]);return m
def emission(m,rgb,image=None):
 nt=m.node_tree;nt.nodes.clear();e=nt.nodes.new('ShaderNodeEmission');e.inputs[0].default_value=(*rgb,1);o=nt.nodes.new('ShaderNodeOutputMaterial');nt.links.new(e.outputs[0],o.inputs[0])
 if image:
  t=nt.nodes.new('ShaderNodeTexImage');t.image=image;nt.links.new(t.outputs['Color'],e.inputs[0])

for key,filename,length,width in MODELS:
 if args and key not in args:continue
 out=ROOT/'output/traffic'/key;out.mkdir(parents=True,exist_ok=True)
 bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.gltf(filepath=str(SOURCES/filename));sc=bpy.context.scene
 meshes=[o for o in sc.objects if o.type=='MESH']
 for o in list(sc.objects):
  if o.type!='MESH':bpy.data.objects.remove(o,do_unlink=True)
 for o in meshes:
  mat=o.matrix_world.copy();o.animation_data_clear();o.parent=None;o.matrix_world=Matrix.Identity(4);o.data=o.data.copy();o.data.transform(mat)
 points=[v.co for o in meshes for v in o.data.vertices];lo=Vector([min(p[i] for p in points) for i in range(3)]);hi=Vector([max(p[i] for p in points) for i in range(3)]);centre=Vector(((lo.x+hi.x)/2,(lo.y+hi.y)/2,lo.z));scale=Vector((width/(hi.x-lo.x),length/(hi.y-lo.y),heights[key]/(hi.z-lo.z)))
 for o in meshes:
  for v in o.data.vertices:
   v.co=(v.co-centre)*scale
   if flip[key]:v.co.x=-v.co.x;v.co.y=-v.co.y
  o.data.update()
 # Source naming varies; explicit material identities keep paint out of glass and lamps.
 records={};category={}
 for m in list(bpy.data.materials):
  bs=m.node_tree.nodes.get('Principled BSDF') if m.use_nodes else None;rgb=tuple(bs.inputs['Base Color'].default_value[:3]) if bs else tuple(m.diffuse_color[:3]);image=None
  if bs and bs.inputs['Base Color'].is_linked:
   node=bs.inputs['Base Color'].links[0].from_node
   if node.type=='TEX_IMAGE':image=node.image
  name=m.name.lower();paint=m.name in paint_names[key];glass=('glass' in name and not any(t in name for t in ['light','red','orange','white','hl_'])) or (key=='crown' and m.name in {'Material.002','Material.003','Material.004','Material.005','Material.006','Material.007','Material.012'}) or (key=='rosa' and m.name=='Metallic')
  plate=any(t in name for t in ['license','number','moji','fuin'])
  if paint:rgb=(1,1,1);image=None
  elif glass:rgb=(.055,.060,.070);image=None
  elif plate:rgb=(1,1,1);image=None
  elif key=='swift' and m.name=='wheel01':rgb=(.018,.021,.025);image=None
  elif max(rgb)<.003 and not image:rgb=(.013,.015,.019)
  records[m.name]=(rgb,image,paint,glass);category[m.name]=128 if paint else 255 if glass else 0;m.use_nodes=True
 # White plates cover existing marks without baking any writing into the traffic sprites.
 white=material('Traffic plain white plate',(1,1,1));records[white.name]=((1,1,1),None,False,False);category[white.name]=0
 for side in [-1,1]:
  bpy.ops.mesh.primitive_cube_add(size=1,location=(0,side*(length/2+.012),.48 if key!='rosa' else .70));o=bpy.context.object;o.name='Plain traffic plate';o.dimensions=(.43,.025,.14);bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);o.data.materials.append(white);o.data.transform(o.matrix_world);o.matrix_world=Matrix.Identity(4);meshes.append(o)
 # Small fitted rear lenses guarantee visible, separate tail/turn signals even
 # when the supplied model stores glass tint in a transmission/atlas shader.
 # Ray-casting fits each lens to the actual rear surface instead of floating it.
 red=material('Traffic red lamp',(.50,.015,.009));amber=material('Traffic amber lamp',(.75,.24,.02))
 for m,rgb in [(red,(.50,.015,.009)),(amber,(.75,.24,.02))]:records[m.name]=(rgb,None,False,False);category[m.name]=0
 bpy.context.view_layer.update();deps=bpy.context.evaluated_depsgraph_get()
 for side in [-1,1]:
  x=side*width*(.325 if key=='swift' else .385)
  for lamp,z in [(red,.72 if key not in ['hiace','rosa'] else .80),(amber,.89 if key not in ['hiace','rosa'] else 1.02)]:
   if key=='swift':z=.76 if lamp==red else .84
   origin=Vector((x,length,z));hit,pos,normal,face,obj,matrix=sc.ray_cast(deps,origin,Vector((0,-1,0)),distance=length)
   y=pos.y+.018 if hit else length*.48
   bpy.ops.mesh.primitive_cube_add(size=1,location=(x,y,z));o=bpy.context.object;o.name='Fitted rear '+lamp.name;o.dimensions=(.085,.022,.085) if key=='swift' else (.15,.035,.12);bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);o.data.materials.append(lamp);o.data.transform(o.matrix_world);o.matrix_world=Matrix.Identity(4);meshes.append(o)
 # Reviewed wheel assemblies. Material names alone are insufficient: some
 # sources reuse rim materials for badges, or join four tyres into one mesh.
 wheel_specs={
  'corolla':([-1.2985,1.3005],.642,.2945,.3085,{'tyre1','rims1'}),
  'swift':([-1.097,1.275],.632,.2625,.291,{'wheel01','rim_chrome'}),
  'crown':([-1.537,1.175],.724,.3055,.312,{'TireRing171Mtl','TireRing171Mtl.001','Wheel0041Mtl','wheel.1','wheel.003','wheel.006','wheel.007'}),
  'hiace':([-1.632,1.4905],.615,.328,.309,{'tire','wheel'}),
  'rosa':([-2.391,1.499],.700,.3785,.363,{'material_0','Metallic_9'}),
 }
 axles,track,radius_z,radius_y,allowed=wheel_specs[key]
 wheel_vertices={};wheel_centres={}
 for o in meshes:
  indices=set()
  for poly in o.data.polygons:
   if o.material_slots and o.material_slots[poly.material_index].material.name in allowed:indices.update(poly.vertices)
  selected={}
  for i in indices:
   p=o.data.vertices[i].co;sy=min(range(2),key=lambda j:abs(p.y-axles[j]));sx=1 if p.x>0 else -1
   c=Vector((sx*track,axles[sy],radius_z))
   # Gate in wheel space; spare wheels, badges, lamps and body remain fixed.
   if abs(p.x)>width*.19 and abs(p.x)<width*.53 and ((p.y-c.y)/radius_y)**2+((p.z-c.z)/radius_z)**2<1.12:
    selected[i]=c;wheel_centres[(sx,sy)]=c
  wheel_vertices[o.name]=selected
 # The Rosa source uses the same pale atlas material for tyres and body details.
 # Assign charcoal only to reviewed road-tyre faces, retaining the silver hubs.
 if key=='rosa':
  rubber=material('Reviewed Rosa road rubber',(.018,.021,.025));records[rubber.name]=((.018,.021,.025),None,False,False);category[rubber.name]=0
  for o in meshes:
   if not any(slot.material and slot.material.name=='material_0' for slot in o.material_slots):continue
   o.data.materials.append(rubber);slot=len(o.data.materials)-1;selected=wheel_vertices[o.name]
   for poly in o.data.polygons:
    if o.material_slots[poly.material_index].material.name=='material_0' and all(i in selected for i in poly.vertices):poly.material_index=slot
 assert len(wheel_centres)==4,(key,'Missing road wheel')
 base={o.name:[v.co.copy() for v in o.data.vertices] for o in meshes}
 print('VALIDATED WHEELS',key,len(wheel_centres),sum(map(len,wheel_vertices.values())),flush=True)
 sc.render.engine='CYCLES';sc.cycles.samples=1;sc.cycles.use_denoising=False;sc.cycles.use_adaptive_sampling=False;sc.cycles.filter_width=.01;sc.render.film_transparent=True;sc.render.dither_intensity=0;sc.render.resolution_x=N*SS;sc.render.resolution_y=N*SS;sc.render.resolution_percentage=100;sc.render.image_settings.file_format='PNG';sc.view_settings.view_transform='Standard';sc.view_settings.look='None';sc.view_settings.exposure=0;sc.view_settings.gamma=1
 data=bpy.data.cameras.new('Traffic orthographic');data.type='ORTHO';data.ortho_scale=N/PPM;data.shift_y=42/N;cam=bpy.data.objects.new('Traffic camera',data);sc.collection.objects.link(cam);sc.camera=cam
 M=Euler((math.radians(60),0,math.radians(135))).to_matrix();R,U,B=[M@Vector(v) for v in ((1,0,0),(0,1,0),(0,0,1))]
 def override(kind):
  m=material('Guide '+kind,(0,0,0));nt=m.node_tree;geo=nt.nodes.new('ShaderNodeNewGeometry');dot=nt.nodes.new('ShaderNodeVectorMath');dot.operation='DOT_PRODUCT';nt.links.new(geo.outputs['Normal'] if kind in ['nx','ny'] else geo.outputs['Position'],dot.inputs[0]);dot.inputs[1].default_value=R if kind=='nx' else U if kind=='ny' else B
  mathn=nt.nodes.new('ShaderNodeMath');mathn.operation='MULTIPLY_ADD';nt.links.new(dot.outputs['Value'],mathn.inputs[0]);mathn.inputs[1].default_value=.5 if kind in ['nx','ny'] else 1/RANGE;mathn.inputs[2].default_value=.5;nt.links.new(mathn.outputs[0],nt.nodes.get('Emission').inputs[0]);return m
 guides={k:override(k) for k in ['nx','ny','depth','far']}
 for pose,sign in [('left',-1),('straight',0),('right',1)]:
  yaw=Matrix.Rotation(math.radians(sign*3),3,'Z');steer=Matrix.Rotation(math.radians(sign*8),3,'Z')
  for frame in range(4):
   spin=Matrix.Rotation(math.radians(frame*90),3,'X')
   for o in meshes:
    for v,p in zip(o.data.vertices,base[o.name]):
     c=wheel_vertices[o.name].get(v.index)
     if c is not None:
      q=p-c;q.y*=radius_z/radius_y;q=spin@q;q.y*=radius_y/radius_z
      v.co=yaw@(c+(steer if c.y<0 else Matrix.Identity(3))@q)
     else:
      v.co=yaw@p
      assert (v.co-yaw@p).length<1e-7
    o.data.update()
   for kind in ['albedo','nx','ny','depth','far','emit','finish']:
    cam.rotation_euler=(M if kind!='far' else M@Matrix.Rotation(math.pi,3,'Y')).to_euler();cam.location=B*(30 if kind!='far' else -30)
    sc.view_settings.view_transform='Standard' if kind=='albedo' else 'Raw';sc.render.image_settings.color_depth='16' if kind in guides else '8';sc.render.image_settings.color_mode='BW' if kind in guides else 'RGBA';sc.view_layers[0].material_override=guides.get(kind)
    if kind in ['albedo','emit','finish']:
     for name,(rgb,image,paint,glass) in records.items():
      m=bpy.data.materials[name]
      if kind=='albedo':emission(m,rgb,image)
      elif kind=='finish':emission(m,(category[name]/255,)*3)
      else:
       # Classify real lamp colour per surface/texture, then gate to vehicle ends.
       emission(m,(0,0,0));nt=m.node_tree
       if paint or glass or name==white.name:continue
       geo=nt.nodes.new('ShaderNodeNewGeometry');sep=nt.nodes.new('ShaderNodeSeparateXYZ');nt.links.new(geo.outputs['Position'],sep.inputs[0])
       def mathop(op,a,b):
        n=nt.nodes.new('ShaderNodeMath');n.operation=op
        for i,v in enumerate([a,b]):
         if isinstance(v,(int,float)):n.inputs[i].default_value=v
         else:nt.links.new(v,n.inputs[i])
        return n.outputs[0]
       front=mathop('LESS_THAN',sep.outputs['Y'],-length*.34);rear=mathop('GREATER_THAN',sep.outputs['Y'],length*.34);height=mathop('MULTIPLY',mathop('GREATER_THAN',sep.outputs['Z'],.20),mathop('LESS_THAN',sep.outputs['Z'],1.6))
       colour=nt.nodes.new('ShaderNodeRGB');colour.outputs[0].default_value=(*rgb,1)
       if image:
        t=nt.nodes.new('ShaderNodeTexImage');t.image=image;colour=t;output=t.outputs['Color']
       else:output=colour.outputs[0]
       channels=nt.nodes.new('ShaderNodeSeparateColor');nt.links.new(output,channels.inputs[0]);r,g,b=[channels.outputs[i] for i in range(3)]
       red=mathop('MULTIPLY',mathop('GREATER_THAN',r,mathop('MULTIPLY',g,1.45)),mathop('GREATER_THAN',r,.17))
       amber=mathop('MULTIPLY',mathop('MULTIPLY',mathop('GREATER_THAN',r,.22),mathop('GREATER_THAN',g,.06)),mathop('GREATER_THAN',g,mathop('MULTIPLY',b,1.8)))
       # Amber must be visibly orange, not neutral metal/white paint.
       amber=mathop('MULTIPLY',amber,mathop('GREATER_THAN',r,mathop('MULTIPLY',g,1.25)))
       if 'amber' in name.lower() or 'orange' in name.lower() or name=='Material__234':red=0.;amber=1.
       elif 'red' in name.lower() or name in ['plt5_1','M_15___Default','Material__245','chrome3']:red=1.;amber=0.
       lamps=any(t in name.lower() for t in ['light','lamp','glass','bulb','metallic_3','plt5','crown.00','crown.01'])
       white_light=mathop('MULTIPLY',front,mathop('GREATER_THAN',g,.30)) if lamps else 0.
       red=mathop('MULTIPLY',mathop('MULTIPLY',red,rear),height);amber=mathop('MULTIPLY',mathop('MULTIPLY',amber,mathop('MAXIMUM',front,rear)),height)
       comb=nt.nodes.new('ShaderNodeCombineColor')
       for i,socket in enumerate([mathop('MULTIPLY',white_light,height),red,amber]):
        if isinstance(socket,(int,float)):comb.inputs[i].default_value=socket
        else:nt.links.new(socket,comb.inputs[i])
       nt.links.new(comb.outputs[0],nt.nodes.get('Emission').inputs[0])
    bpy.context.view_layer.update();sc.render.filepath=str(out/f'{pose}_f{frame}_{kind}.png');bpy.ops.render.render(write_still=True)
   print('TRAFFIC RENDER',key,pose,frame,flush=True)
 (out/'meta.json').write_text(json.dumps({'id':key,'name':filename,'length':length,'width':width,'height':heights[key],'canvas':N,'anchor':[128,170],'ppm':PPM,'depthRange':RANGE,'frames':4,'poses':['left','straight','right']},indent=2))
print('TRAFFIC RENDER COMPLETE',flush=True)
