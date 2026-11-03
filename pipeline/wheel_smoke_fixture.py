"""Export one existing complete wheel to exercise the replacement GLB path."""
import bpy,json
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[1]
folder=ROOT/'output/silvia-modular'
bpy.ops.wm.open_mainfile(filepath=str(folder/'silvia_prepared.blend'))
parts=[o for o in bpy.context.scene.objects if o.type=='MESH' and o.name.startswith('RF_') and 'brake' not in o.name.lower()]
assert parts
centers=json.loads((folder/'wheels.json').read_text())['centers'];center=Vector(centers['(True, False)'])
for o in list(bpy.context.scene.objects):
    if o not in parts:bpy.data.objects.remove(o,do_unlink=True)
for o in parts:
    for v in o.data.vertices:v.co-=center
    o.select_set(True)
bpy.ops.export_scene.gltf(filepath=str(ROOT/'output/reference-wheel.glb'),export_format='GLB',use_selection=True)
print('WHEEL FIXTURE',len(parts),'meshes')
