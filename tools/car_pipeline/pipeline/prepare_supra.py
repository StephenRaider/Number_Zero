import bpy,json,time
from pathlib import Path
from mathutils import Vector,Matrix
import sys as _path_sys; _path_sys.path.insert(0,str(Path(__file__).resolve().parent))
from settings import WORKSPACE as root, CODE, GAME, SOURCES;start=time.perf_counter();bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.gltf(filepath=str(SOURCES/'toyota_supra_mk4__supra_1997__free_download.glb'))
meshes=[o for o in bpy.context.scene.objects if o.type=='MESH'];world={o.name:o.matrix_world.copy() for o in meshes}
for o in meshes:
 o.parent=None;o.matrix_world=Matrix.Identity(4);o.data=o.data.copy();o.data.transform(world[o.name])
for o in list(bpy.context.scene.objects):
 if o.type!='MESH':bpy.data.objects.remove(o,do_unlink=True)
ps=[v.co for o in meshes for v in o.data.vertices];lo=Vector([min(p[i] for p in ps) for i in range(3)]);hi=Vector([max(p[i] for p in ps) for i in range(3)]);scale=4.52/(hi.y-lo.y);center=Vector(((lo.x+hi.x)/2,(lo.y+hi.y)/2,lo.z))
for o in meshes:
 for v in o.data.vertices:v.co=(v.co-center)*scale
 o.data.update()
def mat(name,colour):
 m=bpy.data.materials.new(name);m.use_nodes=True;m.node_tree.nodes.get('Principled BSDF').inputs['Base Color'].default_value=(*colour,1);return m
paint=bpy.data.materials['Paint'];paint.name='body_paint';paint.node_tree.nodes.get('Principled BSDF').inputs['Base Color'].default_value=(.7,.78,.85,1)
window=mat('body_win',(.055,.060,.070));lens=mat('headlight_front_glass',(.6,.64,.68));tyre=mat('wheel_tyre',(.018,.021,.025));rim=mat('wheel_rim',(.45,.46,.48));disc=mat('wheel_disc',(.23,.24,.25));caliper=mat('calliper',(.16,.02,.018));white=mat('Plain white license plate',(1,1,1))
for o in meshes:
 if o.name=='Object_73':o.name='hood'
 if o.name in ['Object_122','Object_151']:o.data.materials.clear();o.data.materials.append(tyre)
 if o.name in ['Object_120','Object_149']:o.data.materials.clear();o.data.materials.append(rim)
 for slot in o.material_slots:
  if slot.material.name=='Glass':slot.material=lens if o.name in ['Object_115','Object_173','Object_135'] else window
  elif slot.material.name=='Brake_Disc':slot.material=disc
  elif slot.material.name=='Brake_Caliper':slot.material=caliper;o.name='calliper_'+o.name
# Fit blank plates to the model rather than putting them beyond the bumper.
bpy.context.view_layer.update();deps=bpy.context.evaluated_depsgraph_get()
for side in [-1,1]:
 z=.31 if side<0 else .43;hit,p,n,face,obj,matrix=bpy.context.scene.ray_cast(deps,Vector((0,side*4,z)),Vector((0,-side,0)),distance=5)
 assert hit,'Plate surface not found'
 bpy.ops.mesh.primitive_cube_add(size=1,location=(0,p.y+side*.009,z));o=bpy.context.object;o.name='Blank front plate' if side<0 else 'Blank rear plate';o.dimensions=(.43,.015,.14);bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);o.data.materials.append(white)
# The lower front indicator inserts are hidden by opaque source bumper pieces.
# Fit small amber lenses to the actual outer face so both front signals work.
amber=mat('orange',(.85,.20,.012));bpy.context.view_layer.update();deps=bpy.context.evaluated_depsgraph_get()
for side in [-1,1]:
 x=side*.63;z=.36;hit,p,n,face,obj,matrix=bpy.context.scene.ray_cast(deps,Vector((x,-4,z)),Vector((0,1,0)),distance=5);assert hit
 bpy.ops.mesh.primitive_cube_add(size=1,location=(x,p.y-.009,z));o=bpy.context.object;o.name='Front amber insert';o.dimensions=(.13,.018,.065);bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);o.data.materials.append(amber)
# This is an experiment source, not a runtime asset.
folder=root/'assets/supra';folder.mkdir(exist_ok=True);bpy.ops.export_scene.gltf(filepath=str(folder/'supra-prepared.glb'),export_format='GLB',export_yup=True)
(root/'output/supra-preparation.json').write_text(json.dumps({'source':str(SOURCES/'toyota_supra_mk4__supra_1997__free_download.glb'),'length_m':4.52,'scale':scale,'seconds':time.perf_counter()-start,'hood':'hood','wheel_materials':['wheel_tyre','wheel_rim','wheel_disc'],'calipers':'stationary calliper units'},indent=2))
print('SUPRA PREPARED',time.perf_counter()-start,flush=True)
