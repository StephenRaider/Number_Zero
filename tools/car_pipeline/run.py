"""Versioned asset tools; inputs and intermediates live in an external workspace."""
import argparse,json,os,shutil,subprocess,sys,time
from datetime import datetime,timezone
from pathlib import Path
HERE=Path(__file__).resolve().parent
GAME=HERE.parents[1]

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('action',choices=['render','verify','sound-check'])
 p.add_argument('--car',choices=['supra','traffic'],default='supra')
 p.add_argument('--workspace',required=True,type=Path,help='External experiment/work directory; never the game checkout')
 p.add_argument('--source-dir',type=Path,help='Directory containing user-supplied source models')
 p.add_argument('--blender',type=Path,help='Blender 5.2 executable, required for rendering')
 p.add_argument('--compiler',type=Path,help='g++ executable for sound-check')
 p.add_argument('--dry-run',action='store_true',help='Print commands without creating files or executing stages')
 a=p.parse_args();work=a.workspace.expanduser().resolve();sources=(a.source_dir or work/'sources').expanduser().resolve()
 if work==GAME or GAME in work.parents:p.error('--workspace must be outside the game checkout')
 if sources==GAME or GAME in sources.parents:p.error('--source-dir must be outside the game checkout')
 code=HERE/'pipeline';steps=[]
 def script(name,*args):return [sys.executable,str(code/name),*map(str,args)]
 def stage(name,cmd):steps.append((name,cmd))
 if a.action=='sound-check':
  compiler=a.compiler or GAME.parent/'toolchain/w64devkit/bin/g++.exe'
  if not compiler.is_file():
   located=shutil.which('g++')
   if not located:p.error('Pass --compiler <g++ executable>')
   compiler=Path(located)
  exe=work/'output/car-pipeline-checks'/('audio_tests.exe' if os.name=='nt' else 'audio_tests')
  stage('compile-audio-checks',[str(compiler),'-std=c++17','-O2','-Wall','-Wextra','-I'+str(GAME/'src'),str(GAME/'tests/audio_tests.cpp'),'-o',str(exe)])
  stage('audio-checks',[str(exe)])
 elif a.action=='verify':stage('verify',script('verify_supra.py' if a.car=='supra' else 'verify_traffic_assets.py'))
 else:
  blender=a.blender or Path(shutil.which('blender') or '')
  if not blender.is_file():p.error('Pass --blender <Blender 5.2 executable>')
  def render(file,*args):return [str(blender.resolve()),'-b','--python-exit-code','1','--python',str(code/file),'--',*args]
  if a.car=='supra':
   source=sources/'toyota_supra_mk4__supra_1997__free_download.glb'
   if not source.is_file():p.error('Missing Supra source: '+str(source))
   stage('prepare',render('prepare_supra.py'))
   stage('neutral-render',render('render_modular.py','supra','8'))
   for name,angle in [('m2','-3'),('m1','-1.5'),('p1','1.5'),('p2','3')]:stage('pitch-'+name,render('render_pitch.py','supra','1','--variant','pitch_'+name,'--pitch',angle))
   stage('neutral-pack',script('pack_modular.py','supra'))
   for name in ['m2','m1','p1','p2']:stage('pack-'+name,script('pack_pitch.py','supra-modular-pitch_'+name))
   stage('export',script('export_game_assets.py','--target',GAME,'--car','supra','--include-pitch'))
   stage('cleanup',script('cleanup_body_views.py','--car','supra'))
   stage('verify',script('verify_supra.py'))
  else:
   stage('traffic-render',render('render_traffic.py'))
   stage('traffic-pack',script('pack_traffic.py'))
   stage('traffic-export',script('export_game_assets.py','--target',GAME,'--traffic'))
   stage('traffic-verify',script('verify_traffic_assets.py'))
 if a.dry_run:
  for name,cmd in steps:print(name+': '+subprocess.list2cmdline(cmd))
  return 0
 work.mkdir(parents=True,exist_ok=True);(work/'output/car-pipeline-checks').mkdir(parents=True,exist_ok=True)
 folder=work/'output/pipeline-runs'/datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%S_%fZ');folder.mkdir(parents=True)
 env=dict(os.environ,NZ_CAR_WORKSPACE=str(work),NZ_CAR_GAME=str(GAME),NZ_CAR_SOURCES=str(sources))
 report={'started_utc':datetime.now(timezone.utc).isoformat(),'action':a.action,'car':a.car,'stages':[]};start=time.perf_counter()
 for name,cmd in steps:
  tick=time.perf_counter();log=folder/(name+'.log')
  try:
   with log.open('w',encoding='utf-8') as out:result=subprocess.run(cmd,cwd=work,env=env,stdout=out,stderr=subprocess.STDOUT)
   status=result.returncode
  except OSError as error:
   log.write_text(str(error));status=1
  report['stages'].append({'name':name,'seconds':round(time.perf_counter()-tick,3),'exit_code':status,'log':log.name})
  report['elapsed_seconds']=round(time.perf_counter()-start,3)
  (folder/'timing.json').write_text(json.dumps(report,indent=2))
  print(name+': '+str(report['stages'][-1]['seconds'])+'s; exit '+str(status),flush=True)
  if status:print('Stopped; inspect '+str(log));return status
 print('Complete. Timing report: '+str(folder/'timing.json'));return 0
if __name__=='__main__':raise SystemExit(main())
