"""Use the established panel-aware packing/cleanup for body-only pitch exports."""
from pathlib import Path
import sys as _path_sys; _path_sys.path.insert(0,str(Path(__file__).resolve().parent))
from settings import WORKSPACE as ROOT, CODE, GAME, SOURCES
source=(CODE/'pack_modular.py').read_text().split('    animation={}')[0]
source=source.replace("for layer in ['body','wheels']", "for layer in ['body']")
source+='''
    sprites=folder/'sprites';sprites.mkdir(exist_ok=True)
    for label,entry in data.items():
        for kind,url in entry['body'].items():
            (sprites/f'{label}_body_{kind}.png').write_bytes(base64.b64decode(url.split(',')[1]))
    print('PACKED PITCH',supplied,flush=True)
'''
exec(compile(source,str(CODE/'pack_modular.py'),'exec'),{'__file__':str(CODE/'pack_modular.py')})
