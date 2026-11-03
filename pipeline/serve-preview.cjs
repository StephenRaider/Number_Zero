// Local preview only. No external network access or filesystem mutations.
const http=require('node:http'),fs=require('node:fs'),path=require('node:path');
const root=path.resolve(__dirname,'../output');
http.createServer((req,res)=>{
 const url=new URL(req.url,'http://127.0.0.1');
 const names={'/output/relight-lab.html':'relight-lab.html','/':'relight-lab.html','/output/customization-lab.html':'customization-lab.html','/output/preview-qa.html':'preview-qa.html'};
 const name=names[url.pathname];if(!name){res.writeHead(404);res.end('Not found');return;}
 res.writeHead(200,{'Content-Type':'text/html; charset=utf-8','Cache-Control':'no-store'});
 fs.createReadStream(path.join(root,name)).pipe(res);
}).listen(Number(process.argv[2]||8765),'127.0.0.1',()=>console.log('Car viewer ready on port '+(process.argv[2]||8765)));
