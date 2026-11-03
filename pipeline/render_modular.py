"""Render modular car layers with Blender 5.2. See MODULAR.md for wheel inputs."""
from pathlib import Path
import sys

ROOT=Path(__file__).resolve().parents[1]
source=(ROOT/'pipeline/render_cars.py').read_text()
source=source.replace("OUT=ROOT/'output'/(KEY+'-detailed' if DETAILED_LAMPS else KEY)","OUT=ROOT/'output'/(KEY+'-modular'+('-'+args[args.index('--variant')+1] if '--variant' in args else ''))")
source=source.replace("VIEWS={'front':-45.,'back':135.}","VIEWS={f'{v}_{p}_{layer}':az for v,az in [('front',-45.),('back',135.)] for p in ['left','straight','right'] for layer in ['body','wheels']}")
# Give the bonnet its own material identity, retaining its original appearance.
insert='''
hood=None
for o in meshes:
    bonnet_vertices=set()
    if KEY=='silvia' and any(m and m.name=='body_paint' for m in o.data.materials):
        adjacency={v.index:set() for v in o.data.vertices}
        for edge in o.data.edges:
            a,b=edge.vertices;adjacency[a].add(b);adjacency[b].add(a)
        unseen=set(adjacency)
        while unseen:
            pending=[unseen.pop()];part=[]
            while pending:
                i=pending.pop();part.append(i)
                for j in adjacency[i]&unseen: unseen.remove(j);pending.append(j)
            points=[o.data.vertices[i].co for i in part]
            lo=Vector([min(p[k] for p in points) for k in range(3)]);hi=Vector([max(p[k] for p in points) for k in range(3)])
            if lo.y<-2.05 and hi.y<-.70 and 1.3<hi.x-lo.x<1.5 and hi.z<.9: bonnet_vertices.update(part)
    for poly in o.data.polygons:
        if not o.data.materials: continue
        m=o.data.materials[poly.material_index]
        if not m or not ('paint' in m.name.lower()): continue
        c=poly.center
        is_hood=('Hood6_' in o.name) if KEY=='civic' else all(i in bonnet_vertices for i in poly.vertices)
        if is_hood:
            if hood is None: hood=m.copy();hood.name='Custom bonnet paint'
            if hood.name not in o.data.materials: o.data.materials.append(hood)
            poly.material_index=o.data.materials.find(hood.name)
'''
source=source.replace('def base_color(m):',insert+'\ndef base_color(m):')
source=source.replace("('paint','chrome','rim','mirror')","('paint','chrome','rim','mirror')")
# The original import bakes transforms; split wheel polygons into four complete units.
extra='''
wheel_objects += [o for o in meshes if o not in wheel_objects and any(s.material and any(t in s.material.name.lower() for t in ['brake_disc','calliper']) for s in o.material_slots)]
wheel_units=[]
for o in list(wheel_objects):
    for key in centers:
        polygons=[p for p in o.data.polygons if ((p.center.x>0),(p.center.y>0))==key]
        if not polygons: continue
        used=sorted({i for p in polygons for i in p.vertices}); lut={old:i for i,old in enumerate(used)}
        me=bpy.data.meshes.new('Wheel unit mesh');me.from_pydata([o.data.vertices[i].co[:] for i in used],[],[[lut[i] for i in p.vertices] for p in polygons])
        for m in o.data.materials: me.materials.append(m)
        for p,old in zip(me.polygons,polygons): p.material_index=old.material_index;p.use_smooth=old.use_smooth
        # Retain UVs: a rim badge or tyre texture must survive detachment.
        for uv in o.data.uv_layers:
            target=me.uv_layers.new(name=uv.name)
            for p,old in zip(me.polygons,polygons):
                for li,oldli in zip(p.loop_indices,old.loop_indices): target.data[li].uv=uv.data[oldli].uv
        me.normals_split_custom_set([o.data.corner_normals[li].vector for old in polygons for li in old.loop_indices])
        unit=bpy.data.objects.new(('R' if key[0] else 'L')+('R' if key[1] else 'F')+'_'+o.name,me);sc.collection.objects.link(unit);wheel_units.append(unit)
    meshes.remove(o);bpy.data.objects.remove(o,do_unlink=True)
wheel_objects=wheel_units;meshes+=wheel_units
base={o.name:[v.co.copy() for v in o.data.vertices] for o in meshes}
replacement=None
if '--wheel-model' in args:
    path=Path(args[args.index('--wheel-model')+1]).resolve()
    previous=set(sc.objects)
    bpy.ops.import_scene.gltf(filepath=str(path))
    imported=[o for o in sc.objects if o not in previous and o.type=='MESH']
    points=[o.matrix_world@v.co for o in imported for v in o.data.vertices]
    low=Vector([min(p[i] for p in points) for i in range(3)]);high=Vector([max(p[i] for p in points) for i in range(3)])
    middle=(low+high)*.5
    diameter=max(high.y-low.y,high.z-low.z)
    if diameter<=0: raise ValueError('Wheel must have its axle along X, with a nonzero Y/Z diameter')
    for o in imported:
        world=o.matrix_world.copy();o.parent=None;o.matrix_world=Matrix.Identity(4)
        for v in o.data.vertices: v.co=(world@v.co-middle)/diameter
    replacement=imported
    for o in wheel_units: meshes.remove(o);bpy.data.objects.remove(o,do_unlink=True)
    wheel_units=[]
    for key,c in centers.items():
        diameter=2*c.z
        for original in imported:
            unit=original.copy();unit.data=original.data.copy();sc.collection.objects.link(unit)
            unit.name=('R' if key[0] else 'L')+('R' if key[1] else 'F')+'_replacement'
            for v in unit.data.vertices:
                p=v.co*diameter
                if not key[0]: p=Matrix.Rotation(math.pi,3,'Z')@p
                v.co=c+p
            wheel_units.append(unit);meshes.append(unit)
    for o in imported: bpy.data.objects.remove(o,do_unlink=True)
    wheel_objects=wheel_units;base={o.name:[v.co.copy() for v in o.data.vertices] for o in meshes}
    for m in bpy.data.materials:
        if m.name not in COL:
            COL[m.name]=base_color(m);SPEC[m.name]=.5
            node=m.node_tree.nodes.get('Principled BSDF') if m.use_nodes else None
            if node and node.inputs['Base Color'].is_linked:
                texture=node.inputs['Base Color'].links[0].from_node
                if texture.type=='TEX_IMAGE': TEX[m.name]=texture.image

base_normals={o.name:[n.vector.copy() for n in o.data.corner_normals] for o in meshes}

def spin(deg):
    pose=view.split('_')[1];layer=view.split('_')[2]
    sign={'left':-1,'straight':0,'right':1}[pose]
    yaw=Matrix.Rotation(math.radians(sign*7),3,'Z')
    roll=Matrix.Rotation(math.radians(sign*-1.4),3,'Y')
    steer=Matrix.Rotation(math.radians(sign*18),3,'Z')
    rotation=Matrix.Rotation(math.radians(deg),3,'X')
    for o in meshes:
        wheel=o in wheel_objects
        o.hide_render=(wheel if layer=='body' else not wheel)
        for v,p in zip(o.data.vertices,base[o.name]):
            if wheel:
                c=centers[(p.x>0,p.y>0)]
                phase=Matrix.Identity(3) if 'calliper' in o.name.lower() else rotation
                v.co=yaw@(c+(steer if c.y<0 else Matrix.Identity(3))@phase@(p-c))
            else: v.co=yaw@(Vector((0,0,.45))+roll@(p-Vector((0,0,.45))))
        # Rotate imported split normals too; editing vertices alone leaves them stale.
        if wheel:
            front=o.name.startswith(('LF_','RF_'))
            rotation_norm=yaw@(steer if front else Matrix.Identity(3))@(Matrix.Identity(3) if 'calliper' in o.name.lower() else rotation)
        else: rotation_norm=yaw@roll
        o.data.normals_split_custom_set([rotation_norm@n for n in base_normals[o.name]])
        o.data.update()

(OUT/'wheels.json').write_text(json.dumps({'axle':'X','units':'metres','front':'-Y','diameter_policy':'match original tyre outside diameter','centers':{str(k):list(c) for k,c in centers.items()},'poses':{'left':[-7,-18,1.4],'straight':[0,0,0],'right':[7,18,-1.4]},'ride_body_z_m':[0,-.04441,-.08882]},indent=2))
'''
source=source.replace('material_ids={m.name:',extra+'\nmaterial_ids={m.name:')
source=source.replace("'RGBA' if kind in ('albedo','emit') else 'BW'", "'RGBA' if kind in ('albedo','emit','material') else 'BW'")
source=source.replace('for frame in range(NFR):','for frame in range(NFR):\n        if view.endswith("_body") and frame>0: continue')
# Guide rays must see through the same lamp lenses as the albedo rays.
source=source.replace('for m in list(bpy.data.materials):\n                    if m.name in material_ids: emission(m,(material_ids[m.name]/255,)*3)', 'for m in list(bpy.data.materials):\n                    if m.name in material_ids: emission(m,(material_ids[m.name]/255,)*3)\n                    if clear_lens(m): transparent(m)')
source=source.replace("('albedo','nx','ny','depth','far','spec','emit','material')","('albedo','nx','ny','depth','far','spec','emit','material')")
# Always detailed, one wheel-spin frame by default; accept N frames for game export.
if '--detailed-lamps' not in sys.argv: sys.argv.append('--detailed-lamps')
exec(compile(source,str(ROOT/'pipeline/render_cars.py'),'exec'))
