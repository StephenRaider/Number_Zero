import bpy
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
bpy.ops.wm.open_mainfile(filepath=str(ROOT/'output/silvia-modular/silvia_prepared.blend'))
for o in bpy.context.scene.objects:
 if o.type!='MESH':continue
 if not any(m and ('paint' in m.name.lower()) for m in o.data.materials):continue
 # Connected components reveal separately modelled panels in the merged GLB.
 adjacency={v.index:set() for v in o.data.vertices}
 for edge in o.data.edges:
  a,b=edge.vertices;adjacency[a].add(b);adjacency[b].add(a)
 unseen=set(adjacency);parts=[]
 while unseen:
  pending=[unseen.pop()];part=[]
  while pending:
   i=pending.pop();part.append(i)
   for j in adjacency[i]&unseen:unseen.remove(j);pending.append(j)
  if len(part)>5:
   pts=[o.data.vertices[i].co for i in part];lo=[round(min(p[k] for p in pts),3) for k in range(3)];hi=[round(max(p[k] for p in pts),3) for k in range(3)];parts.append((len(part),lo,hi))
 print('PANELS',o.name,sorted(parts,reverse=True)[:40],flush=True)
