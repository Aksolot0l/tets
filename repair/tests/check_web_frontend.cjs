// Browser-level mock of the WebView2 message contract; it does not replace a Windows test.
// PLAYWRIGHT_MODULE must point to playwright-core and CHROMIUM_PATH to an installed browser.
const {chromium}=require(process.env.PLAYWRIGHT_MODULE || 'playwright-core');
const path=require('node:path');
const fs=require('node:fs');
const assert=require('node:assert/strict');
(async()=>{
const browser=await chromium.launch({executablePath:process.env.CHROMIUM_PATH,headless:true,args:['--no-sandbox','--single-process','--no-zygote','--disable-gpu']});
const page=await browser.newPage({viewport:{width:1440,height:900}});const errors=[];page.on('pageerror',e=>errors.push(e.message));page.on('console',m=>{if(m.type()==='error')errors.push(m.text())});
const html=fs.readFileSync(path.join(__dirname,'../launcher-web/dist/index.html'));
await page.route('**/*', route => {
  if (route.request().url()==='https://launcher.gayclient.local/') return route.fulfill({status:200,body:html,headers:{'Content-Type':'text/html; charset=utf-8','Content-Security-Policy':"default-src 'none'; script-src 'unsafe-inline'; style-src 'unsafe-inline'; img-src data:; font-src data:; connect-src 'none'; base-uri 'none'; form-action 'none'; frame-src 'none'"}});
  return route.abort();
});
await page.addInitScript(()=>{
 let handlers=[];let profiles=[{id:'0',label:'Мой профиль',nickname:'Тест',version:'1.1.5',aumid:'My.Game!App',createdAt:''}];let selectedId='0';let status='idle';window.__calls=[];
 const state=()=>({profiles,selectedId,status,writable:true});
 const reply=(id,result,ok=true,error='')=>handlers.forEach(fn=>fn({data:{id,result,ok,error}}));
 window.chrome={webview:{addEventListener:(_,fn)=>handlers.push(fn),postMessage:({id,method,args})=>{
  window.__calls.push({method,args});
  setTimeout(()=>{
   if(method==='ready')return reply(id,state());
   if(method==='detectGame')return reply(id,{aumid:'Detected.Game!App'});
   if(method==='launch')return reply(id,{});
   if(method==='saveProfile'){
    if(args.profile.label==='Ошибка')return reply(id,null,false,'Ошибка сохранения');
    if(args.isNew){let profile={...args.profile,id:String(profiles.length)};profiles=[...profiles,profile];selectedId=profile.id}
    else {profiles=profiles.map(p=>p.id===args.profile.id?args.profile:p);selectedId=args.profile.id}
    return reply(id,state());
   }
   if(method==='selectProfile'){selectedId=args.id;return reply(id,state())}
   if(method==='deleteProfile'){profiles=profiles.filter(p=>p.id!==args.id).map((p,i)=>({...p,id:String(i)}));selectedId='0';return reply(id,state())}
   reply(id,{});
  },0);
 }}};
 window.__status = next=>{status=next;handlers.forEach(fn=>fn({data:{event:'status',status}}))};
});
await page.goto('https://launcher.gayclient.local/',{waitUntil:'load'});await page.waitForTimeout(300);
await page.locator('img').first().waitFor();
assert(await page.locator('img').first().evaluate(e=>e.complete && e.naturalWidth>0),'embedded art failed');
assert(await page.getByText('Мой профиль',{exact:true}).count()>0,'native profile not rendered');assert(!(await page.locator('body').innerText()).includes('27 ч 48 мин'),'fake stats');
await page.getByRole('button',{name:'Начать игру'}).click();await page.waitForTimeout(100);assert((await page.evaluate(()=>window.__calls)).some(x=>x.method==='launch'));
await page.evaluate(()=>window.__status('loaded'));await page.getByRole('heading',{name:'DLL загружена'}).waitFor();
await page.getByRole('navigation',{name:'Основная навигация'}).getByRole('button',{name:/^Профили/}).click();
const label=page.getByPlaceholder('Например, Основной'),nickname=page.getByPlaceholder('Ваш ник в игре'),id=page.getByPlaceholder('Package_xxxxx!App');
assert.equal(await label.inputValue(),'Мой профиль');
for(const box of [label,nickname,id]){let m=await box.evaluate(el=>{let r=el.getBoundingClientRect(),i=el.parentElement.querySelector('svg').getBoundingClientRect();return r.left+parseFloat(getComputedStyle(el).paddingLeft)-i.right});assert(m>=10)}
await page.getByRole('button',{name:'Найти игру'}).click();await page.waitForTimeout(100);assert.equal(await id.inputValue(),'Detected.Game!App');
await label.fill('Ошибка');await page.getByRole('button',{name:'Сохранить',exact:true}).click();await page.getByText('Ошибка сохранения').waitFor();assert.equal(await label.inputValue(),'Ошибка','failed save discarded unsaved field');
await label.fill('Обновлённый');await page.getByRole('button',{name:'Сохранить',exact:true}).click();await page.getByText('Профиль сохранён',{exact:true}).waitFor();assert((await page.evaluate(()=>window.__calls)).some(x=>x.method==='saveProfile' && x.args.profile.label==='Обновлённый'));
await page.getByRole('button',{name:'Новый профиль',exact:true}).click();await label.fill('Второй');await nickname.fill('друг');await page.getByRole('button',{name:'Сохранить',exact:true}).click();await page.waitForTimeout(150);assert((await page.evaluate(()=>window.__calls)).some(x=>x.method==='saveProfile' && x.args.isNew));
await page.getByRole('button',{name:/Удалить профиль Обновлённый/}).click();
await page.getByRole('dialog').getByText('Обновлённый',{exact:true}).waitFor();
await page.getByRole('dialog').getByRole('button',{name:'Удалить профиль'}).click();await page.waitForTimeout(150);
assert((await page.evaluate(()=>window.__calls)).some(x=>x.method==='deleteProfile' && x.args.id==='0'),'wrong profile deleted');
for (const name of ['Настройки','Моды','Статистика']){await page.getByRole('navigation',{name:'Основная навигация'}).getByRole('button',{name:new RegExp('^'+name)}).click();await page.waitForTimeout(100);let text=await page.locator('body').innerText();assert(!/[·•—–‐‑‒−﹘﹣－]/.test(text),name+' special separator');if(name!=='Настройки')assert(text.includes('Пока недоступно'));}
console.log('PASS: mock WebView bridge, status, launch, detect, save failure/success, create/delete, CSP, pages, input alignment, no fake stats or separator symbols');console.log('JS errors:',errors);assert.deepEqual(errors,[]);await browser.close();
})().catch(e=>{console.error(e);process.exit(1)});
