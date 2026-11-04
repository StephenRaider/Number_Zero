"""Material-derived rim tint masks; source renders stay outside the game repo."""
import argparse, hashlib, json
from pathlib import Path
import numpy as np
from PIL import Image

CARS = {
    'silvia': ('nissan_silvia_s13', ['wheel_rims_FL']),
    'civic': ('honda_civic_ek9', ['hM_Rim_Main_Max1', 'hM_Rim_NoTint_Max1']),
    'testarossa': ('ferrari_testarossa', ['rims']),
    'supra': ('toyota_supra_a80_sz', ['wheel_rim']),
}
CONTRACT = {
    'version': 1, 'channel': 'L', 'range': [0, 255],
    'meaning': '0 preserve stock pixel; 255 tint rim; intermediate values blend tint coverage.',
    'sampling': 'point; linear data, never sRGB',
    'protected': 'Tyres, brakes, calipers, separate badges/bolts and transparent gaps.',
    'blur': 'Coverage-weighted average of the eight categorical spin masks.',
}

def blocks(a, size=128):
    h, w = a.shape[:2]
    if h != w or h % size: raise ValueError('Expected square supersampled material render')
    ss = h // size
    return np.moveaxis(a.reshape(size, ss, size, ss, *a.shape[2:]), 1, 2).reshape(size, size, ss*ss, *a.shape[2:])

def material_mask(material, raw_albedo, rim_ids, packed_alpha):
    ids = material[..., 0] if material.ndim == 3 else material
    covered = raw_albedo[..., 3] > 0
    if material.ndim == 3 and material.shape[2] == 4: covered &= material[..., 3] > 0
    mb, cb = blocks(ids), blocks(covered)
    counts = np.stack([((mb == i) & cb).sum(-1) for i in range(int(ids.max())+1)], -1)
    # Same categorical material vote as the body/neutral wheel packer. Do not
    # dilate into tyres or fill holes based on brightness or a circular shape.
    winner = counts.argmax(-1)
    protected = counts.copy();protected[..., rim_ids] = 0
    unambiguous = counts.max(-1) > protected.max(-1)
    return (np.isin(winner, rim_ids) & unambiguous & cb.any(-1) & (packed_alpha > 0)).astype(np.uint8)*255

def blur_mask(masks, coverage, alpha):
    denominator = np.maximum(np.sum(coverage, axis=0), 1)
    blur = np.round(np.sum(np.array(masks, dtype=float), axis=0)/denominator).clip(0,255).astype(np.uint8)
    blur[alpha == 0] = 0
    return blur

def generate(source, key):
    ids = json.loads((source/'material_ids.json').read_text())
    names = CARS[key][1]
    missing = [name for name in names if name not in ids]
    if missing: raise ValueError(f'{key}: missing explicit rim material assignments: {missing}')
    rim_ids = [ids[name] for name in names]
    output = {}; stats = {}
    for view in ['front', 'back']:
        for pose in ['left', 'straight', 'right']:
            label = f'{view}_{pose}'; masks = []; coverage = []
            for frame in range(8):
                prefix = f'{label}_wheels_f{frame}'
                material = np.array(Image.open(source/f'{prefix}_material.png'))
                raw = np.array(Image.open(source/f'{prefix}_albedo.png').convert('RGBA'))
                packed = np.array(Image.open(source/'sprites'/f'{prefix}_albedo.png').convert('RGBA'))
                mask = material_mask(material, raw, rim_ids, packed[..., 3])
                if not mask.any(): raise ValueError(f'Empty rim mask: {key}/{prefix}')
                masks.append(mask);coverage.append(packed[..., 3] > 0)
                output[f'sprites/{prefix}_rim_mask.png'] = mask
            alpha = np.array(Image.open(source/'sprites'/f'{label}_wheels_f8_albedo.png').convert('RGBA'))[..., 3]
            blur = blur_mask(masks, coverage, alpha)
            output[f'sprites/{label}_wheels_f8_rim_mask.png'] = blur
            stats[label] = {'spin_pixels': [int(np.count_nonzero(m)) for m in masks], 'blur_pixels': int(np.count_nonzero(blur))}
    for name, mask in output.items():
        path = source/name;path.parent.mkdir(parents=True, exist_ok=True)
        Image.fromarray(mask).save(path, optimize=True)
    (source/'rim-mask-report.json').write_text(json.dumps({'materials': names, 'contract': CONTRACT, 'poses': stats}, indent=2)+'\n')
    return output

def publish(source, destination, key, output):
    manifest_path = destination/'manifest.json'
    manifest = json.loads(manifest_path.read_text())
    if manifest['id'] != CARS[key][0]: raise ValueError('Car manifest mismatch')
    # Validate every destination before the first write, preserving game-side edits.
    for name in output:
        target = destination/name;raw = (source/name).read_bytes()
        if target.exists() and target.read_bytes() != raw:
            if hashlib.sha256(target.read_bytes()).hexdigest() != manifest['files'].get(name):
                raise ValueError(f'Unrecorded game-side mask edit: {target}')
    for name in output:
        raw = (source/name).read_bytes();(destination/name).write_bytes(raw)
        manifest['files'][name] = hashlib.sha256(raw).hexdigest()
    for label, pose in manifest['poses'].items():
        for frame, entry in enumerate(pose['stock_wheels']['frames']):
            entry['rim_mask'] = f'sprites/{label}_wheels_f{frame}_rim_mask.png'
    manifest['rim_tint'] = dict(CONTRACT, materials=CARS[key][1])
    manifest['wheel_asset_status'] = 'Car-specific stock wheel shapes/animation with separate rim-only tint masks; wheel-design selection UI is not implemented.'
    manifest_path.write_text(json.dumps(manifest, indent=2)+'\n', encoding='utf-8')

def verify(source, destination, key, output):
    manifest = json.loads((destination/'manifest.json').read_text())
    for name, expected in output.items():
        path = destination/name
        image = Image.open(path)
        if image.mode != 'L' or image.size != (128,128): raise ValueError(f'Invalid rim mask format: {path}')
        actual = np.array(image)
        alpha = np.array(Image.open(destination/name.replace('_rim_mask.png','_albedo.png')).convert('RGBA'))[...,3]
        if not np.array_equal(actual, expected) or np.any(actual[alpha == 0]): raise ValueError(f'Invalid coverage: {path}')
        if hashlib.sha256(path.read_bytes()).hexdigest() != manifest['files'].get(name): raise ValueError(f'Mask checksum mismatch: {path}')
    for label, pose in manifest['poses'].items():
        for frame, entry in enumerate(pose['stock_wheels']['frames']):
            if entry.get('rim_mask') != f'sprites/{label}_wheels_f{frame}_rim_mask.png': raise ValueError('Unregistered rim mask')
    print(f'VERIFIED {key}: {len(output)} material-derived rim masks; all views/spin/blur; no transparent coverage', flush=True)

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--workspace', type=Path, required=True)
    p.add_argument('--game', type=Path, required=True)
    p.add_argument('--car', choices=[*CARS, 'all'], default='all')
    p.add_argument('--publish', action='store_true')
    p.add_argument('--verify', action='store_true')
    a = p.parse_args()
    if a.workspace.resolve() == a.game.resolve() or a.game.resolve() in a.workspace.resolve().parents: p.error('Source workspace must be outside game checkout')
    for key in CARS if a.car == 'all' else [a.car]:
        source = a.workspace/'output'/f'{key}-modular';destination = a.game/'assets/cars'/CARS[key][0]
        output = generate(source, key)
        if a.publish: publish(source, destination, key, output)
        if a.verify: verify(source, destination, key, output)
        print(f'RIM MASKS {key}: {len(output)} PNGs', flush=True)
if __name__ == '__main__': main()
