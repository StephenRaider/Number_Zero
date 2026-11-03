"""Small inspection render using original diffuse colours and textures."""
import bpy,sys
from pathlib import Path
from mathutils import Vector
src,dst=sys.argv[sys.argv.index('--')+1:]
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=src)
sc=bpy.context.scene
meshes=[o for o in sc.objects if o.type=='MESH']
points=[o.matrix_world@Vector(p) for o in meshes for p in o.bound_box]
low=Vector([min(p[i] for p in points) for i in range(3)])
high=Vector([max(p[i] for p in points) for i in range(3)])
centre=(low+high)/2;size=max(high-low)
for m in bpy.data.materials:
    if not m.use_nodes: continue
    nt=m.node_tree;bsdf=next((n for n in nt.nodes if n.type=='BSDF_PRINCIPLED'),None)
    if not bsdf: continue
    e=nt.nodes.new('ShaderNodeEmission');e.inputs['Color'].default_value=bsdf.inputs['Base Color'].default_value
    if bsdf.inputs['Base Color'].is_linked:nt.links.new(bsdf.inputs['Base Color'].links[0].from_socket,e.inputs['Color'])
    out=next(n for n in nt.nodes if n.type=='OUTPUT_MATERIAL');nt.links.new(e.outputs[0],out.inputs['Surface'])
camdata=bpy.data.cameras.new('Inspection');camdata.type='ORTHO';camdata.ortho_scale=size*1.1
cam=bpy.data.objects.new('Inspection',camdata);sc.collection.objects.link(cam)
cam.location=centre+Vector((4,-6,3)).normalized()*size*3
cam.rotation_euler=(centre-cam.location).to_track_quat('-Z','Y').to_euler();sc.camera=cam
sc.render.engine='CYCLES';sc.cycles.samples=1;sc.cycles.use_denoising=False
sc.render.resolution_x=640;sc.render.resolution_y=480;sc.render.resolution_percentage=100
sc.render.film_transparent=True;sc.view_settings.view_transform='Standard'
sc.render.image_settings.file_format='PNG';sc.render.filepath=dst
bpy.ops.render.render(write_still=True)
