"""Package the current game-facing maps without colliding car folder names."""
from pathlib import Path
from zipfile import ZipFile, ZIP_DEFLATED
ROOT=Path(__file__).resolve().parents[1]
with ZipFile(ROOT/'output/car-sprite-maps.zip','w',ZIP_DEFLATED) as archive:
    for car in ('silvia','civic','silvia-detailed','civic-detailed'):
        folder=ROOT/'output'/car
        for set_name in ('sprites','sprites-before','sprites-improved','sprites-wheels'):
            for image in sorted((folder/set_name).glob('*.png')):
                archive.write(image,f'{car}/{set_name}/{image.name}')
        archive.write(folder/'meta.json',f'{car}/meta.json')
        if (folder/'meta.improved.json').exists(): archive.write(folder/'meta.improved.json',f'{car}/meta.improved.json')
    archive.write(ROOT/'README.md','README.md')
    archive.write(ROOT/'pipeline/MODULAR.md','MODULAR.md')
    for car in ('silvia-modular','civic-modular'):
        folder=ROOT/'output'/car
        for name in ('meta.json','wheels.json'):archive.write(folder/name,f'{car}/{name}')
        for sub in ('sprites','masks'):
            for path in sorted((folder/sub).glob('*.png')):archive.write(path,f'{car}/{sub}/{path.name}')
print('PACKAGED',ROOT/'output/car-sprite-maps.zip')
