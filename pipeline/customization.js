(()=>{
 const root=document.getElementById('car-customizer');
 const DATA=__PREVIEW_DATA__,IMAGE_DATA=__PREVIEW_IMAGES__;
 const cv=root.querySelector('canvas'),ctx=cv.getContext('2d');
 const controls=Object.fromEntries([...root.querySelectorAll('[data-control]')].map(e=>[e.dataset.control,e]));
 let map=null,layers=null,request=0;const cleanup='after';
 const state=()=>Object.fromEntries(Object.entries(controls).map(([k,e])=>[k,e.type==='checkbox'?e.checked:e.value]));
 function restore(s){if(!s||s.version!==2)return;for(const [k,v] of Object.entries(s))if(controls[k]){if(controls[k].type==='checkbox')controls[k].checked=!!v;else controls[k].value=String(v);}}
 restore(window.openai?.widgetState?.modelContent);
 const cache=new Map();
 function loadImage(id){if(cache.has(id))return cache.get(id);const promise=new Promise((resolve,reject)=>{const im=new Image();im.onload=()=>{const c=document.createElement('canvas');c.width=c.height=128;const x=c.getContext('2d');x.drawImage(im,0,0);resolve(x.getImageData(0,0,128,128).data);};im.onerror=()=>reject(new Error('Unable to load car layer'));im.src=IMAGE_DATA[id];});cache.set(id,promise);return promise;}
 async function decode(entry){const [a,n,p,q,e,m]=await Promise.all(['albedo','normal','pack','pack2','emit','mask'].map(k=>loadImage(entry[k])));const normal=new Float32Array(128*128*3),depth=new Float32Array(128*128),far=new Float32Array(128*128);
 for(let i=0;i<128*128;i++){const j=i*4,k=i*3;normal[k]=(n[j]*256+n[j+1])/65535*2-1;normal[k+1]=(n[j+2]*256+q[j])/65535*2-1;normal[k+2]=Math.sqrt(Math.max(0,1-normal[k]**2-normal[k+1]**2));depth[i]=((p[j]*256+p[j+1])/65535-.5)*5.2;far[i]=((q[j+1]*256+q[j+2])/65535-.5)*5.2;}
 return {a,n:normal,p,e,m,d:depth,f:far};}
 async function load(){const id=++request,s=state();const entry=DATA[s.car][s.view+'_'+s.pose];const [body,wheels]=await Promise.all([decode(entry.body),decode(entry.wheels)]);if(id!==request)return;layers={body,wheels};compose();}
 function colour(hex){return [1,3,5].map(i=>parseInt(hex.slice(i,i+2),16));}
 function compose(){if(!layers)return;const s=state();root.querySelector('[data-secondary]').hidden=s.car!=='silvia'||!s.twotone;root.querySelector('[data-two-tone]').hidden=s.car!=='silvia';controls.carbon.disabled=!s.hood;
 root.querySelector('[data-status]').textContent=controls.car.selectedOptions[0].text+', '+s.view+', '+s.pose+', '+controls.ride.selectedOptions[0].text;
 const size=128*128;map={a:new Uint8ClampedArray(size*4),n:new Float32Array(size*3),p:new Uint8ClampedArray(size*4),e:new Uint8ClampedArray(size*4),d:new Float32Array(size),f:new Float32Array(size)};
 map.d.fill(-99);map.f.fill(99);const drop=Number(s.ride),dz=-drop/(26*Math.cos(Math.PI/6));
 const paints=[colour(s.paint),colour(s.twotone?s.secondary:s.paint),s.hood?[29,32,36]:colour(s.paint)];
 const original=s.car==='silvia'?[[255,234,186],[174,174,174],[255,234,186]]:[[219,229,237],[219,229,237],[219,229,237]];
 for(const layer of ['body','wheels']){
 if(s.layer!=='complete'&&s.layer!==layer)continue;
 const src=layers[layer],offset=layer==='body'?drop:0,depthOffset=layer==='body'?dz*.5:0;
 for(let y=0;y<128-offset;y++)for(let x=0;x<128;x++){const si=y*128+x,i=(y+offset)*128+x,sj=si*4,j=i*4,sk=si*3,k=i*3;if(src.a[sj+3]<128)continue;
 const depth=src.d[si]+depthOffset;if(depth<map.d[i])continue;
 map.d[i]=depth;map.f[i]=src.f[si]+depthOffset;
 for(let c=0;c<4;c++){map.a[j+c]=src.a[sj+c];map.p[j+c]=src.p[sj+c];map.e[j+c]=src.e[sj+c];}
 for(let c=0;c<3;c++)map.n[k+c]=src.n[sk+c];
 if(layer==='body'){let channel=-1;for(let c=0;c<3;c++)if(src.m[sj+c]>127)channel=c;
 if(channel>=0)for(let c=0;c<3;c++){let value=paints[channel][c]*src.a[sj+c]/original[channel][c];if(channel===2&&s.hood&&s.carbon)value*=((x+y)%4<2?.78:1);map.a[j+c]=Math.min(255,value);}}
 }}draw();}
 function themeRGB(name){const sample=document.createElement('span');sample.style.color='var('+name+')';root.append(sample);const c=getComputedStyle(sample).color;sample.remove();const tmp=document.createElement('canvas');tmp.width=tmp.height=1;const t=tmp.getContext('2d');t.fillStyle=c;t.fillRect(0,0,1,1);return t.getImageData(0,0,1,1).data;}
 __DRAW_FUNCTION__
 function changed(key){if(key==='car'){controls.paint.value=controls.car.value==='silvia'?'#ffeaba':'#dbe5ed';}if(['car','view','pose'].includes(key))load().catch(report);else compose();window.openai?.setWidgetState({modelContent:{...state(),version:2}}).catch(()=>{});}
 function report(error){root.querySelector('[data-status]').textContent=error.message;console.error(error);}
 for(const [key,e] of Object.entries(controls))e.addEventListener('input',()=>changed(key));
 window.addEventListener('openai:set_globals',e=>{restore(e.detail?.globals?.widgetState?.modelContent);load().catch(report);});
 new MutationObserver(()=>draw()).observe(document.documentElement,{attributes:true,attributeFilter:['class','style','data-theme']});
 const blinkTimer=setInterval(()=>{if(!root.isConnected){clearInterval(blinkTimer);return;}if(controls.hazards.checked)draw();},400);
 load().catch(report);
})();
