"""Blender body-only suspension pitch samples; reuse all existing wheel frames."""
from pathlib import Path
import sys as _path_sys; _path_sys.path.insert(0,str(Path(__file__).resolve().parent))
from settings import WORKSPACE as ROOT, CODE, GAME, SOURCES

def run_pitch(source,filename,mode):
    source=source.replace("for layer in ['body','wheels']", "for layer in ['body']")
    source=source.replace("    roll=Matrix.Rotation(math.radians(sign*-1.4),3,'Y')", "    roll=Matrix.Rotation(math.radians(sign*-1.4),3,'Y')\n    pitch=Matrix.Rotation(math.radians(-float(args[args.index('--pitch')+1])),3,'X')")
    source=source.replace("roll@(p-Vector((0,0,.45)))", "roll@pitch@(p-Vector((0,0,.45)))")
    source=source.replace('rotation_norm=yaw@roll', 'rotation_norm=yaw@roll@pitch')
    exec(compile(source,filename,mode),{'__file__':str(CODE/'render_cars.py'),'__name__':'__main__'})

bootstrap=(CODE/'render_modular.py').read_text().replace("exec(compile(source,str(CODE/'render_cars.py'),'exec'))", "run_pitch(source,str(CODE/'render_cars.py'),'exec')")
exec(compile(bootstrap,str(CODE/'render_modular.py'),'exec'),{'__file__':str(CODE/'render_modular.py'),'run_pitch':run_pitch})
