import bpy, json
from pathlib import Path
from mathutils import Vector
ROOT = Path(__file__).resolve().parents[1]
for key, src in [('silvia', ROOT/'assets/silvia/silvia.glb'), ('civic', ROOT/'assets/civic/source/2000 Honda Civic Type-R (EK9).glb')]:
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=str(src))
    meshes = [o for o in bpy.context.scene.objects if o.type == 'MESH']
    pts = [o.matrix_world@Vector(c) for o in meshes for c in o.bound_box]
    bounds = [[min(p[i] for p in pts), max(p[i] for p in pts)] for i in range(3)]
    report = {'bounds': bounds, 'materials': [], 'objects': []}
    for m in bpy.data.materials:
        obs = [o for o in meshes if m.name in [s.material.name for s in o.material_slots if s.material]]
        ps = [o.matrix_world@Vector(c) for o in obs for c in o.bound_box]
        report['materials'].append({'name':m.name,'objects':len(obs),'bounds':[[round(min(p[i] for p in ps),4),round(max(p[i] for p in ps),4)] for i in range(3)] if ps else []})
    for o in meshes:
        ps = [o.matrix_world@Vector(c) for c in o.bound_box]
        report['objects'].append({'name':o.name,'materials':[s.material.name if s.material else '' for s in o.material_slots], 'center':[sum(p[i] for p in ps)/8 for i in range(3)], 'size':[max(p[i] for p in ps)-min(p[i] for p in ps) for i in range(3)]})
    out = ROOT/'assets'/key/'inspection.json'
    out.write_text(json.dumps(report,indent=2))
    print(key, 'bounds', bounds, 'materials', report['materials'], flush=True)
