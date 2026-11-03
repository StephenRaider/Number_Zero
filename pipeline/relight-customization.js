// Feed customized, depth-composited maps into the ORIGINAL Relight Lab shader.
const modularDecodeCache=new WeakMap();
async function modularDecode(entry){
 if(modularDecodeCache.has(entry))return modularDecodeCache.get(entry);
 const pending=Promise.all(['albedo','normal','pack','pack2','emit','mask'].map(async k=>pixels(await loadImg(entry[k]),128)));
 modularDecodeCache.set(entry,pending);return pending;
}
async function getSet(view,st){
 const car=state.car||'silvia',pose=$('pose').value,label=view+'_'+pose,source=MODULAR[car],entry=source.data[label];
 const body=await modularDecode(entry.body),wheelFrames=await Promise.all(source.animation[label].map(modularDecode));
 const paint=hexRGB($('bodyPaint').value),lower=hexRGB($('twoTone').checked?$('lowerPaint').value:$('bodyPaint').value);
 const hood=$('blackHood').checked?[29,32,36]:paint,paints=[paint,lower,hood];
 const original=car==='silvia'?[[255,234,186],[174,174,174],[255,234,186]]:[[219,229,237],[219,229,237],[219,229,237]];
 const drop=Number($('rideHeight').value),depthOffset=-drop/(26*Math.cos(Math.PI/6))*.5;
 const part=$('partView').value,raw={a:[],n:[],p:[],q:[],e:[]};
 for(let fi=0;fi<9;fi++){
  const output=Array.from({length:5},()=>new Uint8ClampedArray(128*128*4));const depths=new Float32Array(128*128);depths.fill(-99);
  for(const [name,src] of [['body',body],['wheels',wheelFrames[Math.min(fi,wheelFrames.length-1)]]]){
   if(part!=='complete'&&part!==name)continue;
   const offset=name==='body'?drop:0,dz=name==='body'?depthOffset:0;
   for(let y=0;y<128-offset;y++)for(let x=0;x<128;x++){
    const si=(y*128+x)*4,i=(y+offset)*128+x,j=i*4;if(src[0][si+3]<128)continue;
    const depth=((src[2][si]*256+src[2][si+1])/65535-.5)*5.2+dz;
    if(depth<depths[i])continue;depths[i]=depth;
    for(let k=0;k<5;k++)for(let c=0;c<4;c++)output[k][j+c]=src[k][si+c];
    if(name==='body'){
     let channel=-1;for(let c=0;c<3;c++)if(src[5][si+c]>127)channel=c;
     if(channel>=0)for(let c=0;c<3;c++){
      let value=paints[channel][c]*src[0][si+c]/original[channel][c];
      if(channel===2&&$('blackHood').checked&&$('carbonHood').checked)value*=((x+y)%4<2?.8:1);
      output[0][j+c]=Math.min(255,value);
     }
     const d=Math.max(0,Math.min(65535,Math.round((depth/5.2+.5)*65535)));
     const far=((src[3][si+1]*256+src[3][si+2])/65535-.5)*5.2+dz;
     const f=Math.max(0,Math.min(65535,Math.round((far/5.2+.5)*65535)));
     output[2][j]=d>>8;output[2][j+1]=d&255;output[3][j+1]=f>>8;output[3][j+2]=f&255;
    }
   }
  }
  for(const [k,v] of Object.entries({a:0,n:1,p:2,q:3,e:4}))raw[k].push(output[v]);
 }
 return raw;
}
function hexRGB(hex){return [1,3,5].map(i=>parseInt(hex.slice(i,i+2),16));}
function syncCustomization(){
 $('lowerPaintRow').hidden=state.car!=='silvia'||!$('twoTone').checked;
 $('twoToneRow').hidden=state.car!=='silvia';$('carbonHood').disabled=!$('blackHood').checked;
}
for(const id of ['bodyPaint','lowerPaint','blackHood','carbonHood','twoTone','pose','rideHeight','partView'])$(id).addEventListener('input',()=>{syncCustomization();setSprite();});
$('segCar').addEventListener('click',()=>{ $('bodyPaint').value=state.car==='silvia'?'#ffeaba':'#dbe5ed';syncCustomization();setSprite();});
const persistIds=['bodyPaint','lowerPaint','blackHood','carbonHood','twoTone','pose','rideHeight','partView','tailon','headon','brake','hazard','lighton','shadow','gx','gy','gz','amb','spec','shin','bands','inten','rad','lcol'];
function restoreLab(saved){
 if(saved?.version!==3)return;
 for(const id of persistIds)if(saved[id]!==undefined){const el=$(id);if(el.type==='checkbox')el.checked=!!saved[id];else el.value=String(saved[id]);}
 if(['silvia','civic'].includes(saved.car))state.car=saved.car;
 if(['front','back'].includes(saved.view))state.view=saved.view;
 for(const [id,value] of [['segCar',state.car],['segView',state.view]])for(const b of $(id).querySelectorAll('button'))b.setAttribute('aria-pressed',b.dataset.v===value);
 ASSETS=selectedAssets();
}
restoreLab(window.openai?.widgetState?.modelContent);
function saveLab(){const saved={version:3,car:state.car,view:state.view};for(const id of persistIds){const el=$(id);saved[id]=el.type==='checkbox'?el.checked:el.value;}window.openai?.setWidgetState({modelContent:saved}).catch(()=>{});}
document.addEventListener('input',saveLab);
for(const id of ['segCar','segView'])$(id).addEventListener('click',saveLab);
window.addEventListener('openai:set_globals',event=>{restoreLab(event.detail?.globals?.widgetState?.modelContent);syncCustomization();setSprite();});
syncCustomization();
