// Painel web do Claudinho: a pagina que a propria placa serve em /painel.
// Estatica (nada pessoal aqui); os dados vem de /painel.json, que pede a chave
// de leitura (o claudinho.sh web grava na placa e abre o endereco com ela).
// A pagina le a chave do "#k=" do endereco: o fragmento nao vai na requisicao
// da pagina, so na do JSON, como ?k=.
#pragma once
static const char PAINEL_HTML[] PROGMEM = R"HTML(<!doctype html>
<html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>Claudinho</title>
<style>
:root{--f:#1d1c1c;--c:#2a2828;--t:#f2f3f4;--a:#a5a09d;--o:#d0ad6c;--ok:#7bbf7b;--al:#d9a25a;--cr:#c96f6f;--tr:#454343}
*{box-sizing:border-box}body{margin:0;background:var(--f);color:var(--t);font:15px system-ui,sans-serif;padding:16px}
h1{font-size:20px;margin:0 0 2px}h1 b{color:var(--o)}.sub{color:var(--a);font-size:13px;margin-bottom:14px}
.grade{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:10px;margin-bottom:10px}
.c{background:var(--c);border-radius:12px;padding:12px}.r{font-size:11px;letter-spacing:.08em;text-transform:uppercase;color:var(--a)}
.v{font-size:20px;font-weight:700;margin-top:4px}.m{font-size:12px;color:var(--a);margin-top:2px}
.duas{display:grid;grid-template-columns:repeat(auto-fit,minmax(280px,1fr));gap:10px}
.l{display:flex;justify-content:space-between;margin:10px 0 4px}.b{height:8px;border-radius:4px;background:var(--tr);overflow:hidden}
.b i{display:block;height:100%}table{width:100%;border-collapse:collapse;font-size:13px}td{padding:5px 4px;border-top:1px solid var(--tr)}
td:first-child{color:var(--a);white-space:nowrap}
.aj{display:flex;flex-wrap:wrap;gap:8px;align-items:center;margin:10px 0}.aj span{color:var(--a);font-size:13px;min-width:90px}
button,select{background:#363435;color:var(--t);border:1px solid var(--tr);border-radius:8px;padding:6px 10px;font:inherit;font-size:13px;cursor:pointer}
button:hover{border-color:var(--o)}button.on{border-color:var(--ok);color:var(--ok)}button.perigo:hover{border-color:var(--cr);color:var(--cr)}
input[type=range]{flex:1;min-width:120px;accent-color:var(--o)}input[type=color]{width:44px;height:32px;border:0;background:none;padding:0}
#aviso{position:fixed;bottom:16px;left:50%;transform:translateX(-50%);background:var(--c);border:1px solid var(--o);padding:8px 14px;border-radius:8px;display:none}#erro{display:none;background:var(--cr);color:#fff;padding:10px;border-radius:8px;margin-bottom:10px}
</style></head><body>
<h1>Claudinho <b id="t0"></b></h1><div class="sub" id="sub"></div>
<div id="erro"></div>
<div class="grade">
 <div class="c"><div class="r" id="t1"></div><div class="v" id="cara">-</div><div class="m" id="sess"></div></div>
 <div class="c"><div class="r">Wi-Fi</div><div class="v" id="wifi">-</div><div class="m" id="ip"></div></div>
 <div class="c"><div class="r" id="t2"></div><div class="v" id="liga">-</div><div class="m" id="mem"></div></div>
 <div class="c" id="cbat" style="display:none"><div class="r" id="t3"></div><div class="v" id="bat">-</div><div class="m" id="batm"></div></div>
 <div class="c" id="cimp" style="display:none"><div class="r" id="t4"></div><div class="v" id="imp">-</div><div class="m" id="impm"></div></div>
</div>
<div class="duas">
 <div class="c"><div class="r" id="t5"></div><div id="usos"></div></div>
 <div class="c"><div class="r" id="t6"></div><table id="hist"></table></div>
</div>
<div class="c" style="margin-top:10px"><div class="r" id="t7"></div>
 <div class="aj" id="ajsom"><span id="l1"></span><button id="bsom"></button><input type="range" id="vol" min="10" max="200" step="10"><b id="vvol"></b>
  <select id="humor"></select><button id="btest"></button></div>
 <div class="aj"><span id="l2"></span><input type="color" id="cor"><button id="bcor"></button><button id="bsalva"></button><button id="bpal"></button></div>
 <div class="aj"><span id="l3"></span><select id="idi"><option value="en">English</option><option value="pt-BR">Português</option></select></div>
 <div class="aj"><span id="l4"></span><button id="bcons"></button><button id="bimp"></button><button id="bvelha"></button><button id="bgen"></button><button id="bdemo"></button></div>
 <div class="aj"><span id="l5"></span><button id="breini" class="perigo"></button><button id="bdesl" class="perigo"></button></div>
</div>
<div id="aviso"></div>
<script>
const PT=(navigator.language||"").toLowerCase().startsWith("pt");
const T=PT?{t0:"Status",t1:"Cara agora",t2:"Ligado há",t3:"Bateria",t4:"Impressora",t5:"Consumo do plano",t6:"Últimos eventos",
 h5:"Janela de 5 horas",d7:"Janela de 7 dias",ctx:"Contexto",sess:"sessões abertas",ren:"renova em ",carga:"carregando",
 bat:"na bateria",livre:"KB livres",chave:"Chave inválida ou ausente: rode  claudinho.sh web  para abrir o painel.",
 sem:"Sem resposta do Claudinho.",t7:"Ajustes",l1:"Som",l2:"Cor do rosto",l3:"Idioma da tela",l4:"Mostrar",l5:"Placa",
 ligado:"ligado",mudo:"mudo",testar:"Testar",aplicar:"Aplicar",salvar:"Salvar",paleta:"Paleta na tela",consumo:"Consumo",impressora:"Impressora",
 velha:"Jogo da velha",genius:"Genius",demo:"Demonstração",reiniciar:"Reiniciar",desligar:"Desligar",
 conf:"Desligar o Claudinho? Ele só volta apertando BOOT (ou RESET) na placa.",feito:"Feito!",negado:"Não permitido pelo painel",
 humores:["feliz","pergunta","sono","bravo","pronto","pensando","empolgado","preocupado","susto","zonzo"],
 caras:["dormindo","neutro","pensando","trabalhando","esperando você","terminou","feliz","empolgado","preocupado","susto","zonzo","cansado","suando","bravo","triste","desconfiado"]}
:{t0:"Status",t1:"Face now",t2:"Up for",t3:"Battery",t4:"Printer",t5:"Plan usage",t6:"Recent events",
 h5:"5-hour window",d7:"7-day window",ctx:"Context",sess:"open sessions",ren:"resets in ",carga:"charging",
 bat:"on battery",livre:"KB free",chave:"Missing or wrong key: run  claudinho.sh web  to open the panel.",
 sem:"No answer from Claudinho.",t7:"Settings",l1:"Sound",l2:"Face color",l3:"Screen language",l4:"Show",l5:"Board",
 ligado:"on",mudo:"muted",testar:"Test",aplicar:"Apply",salvar:"Save",paleta:"Palette on screen",consumo:"Usage",impressora:"Printer",
 velha:"Tic-tac-toe",genius:"Simon",demo:"Demo",reiniciar:"Restart",desligar:"Turn off",
 conf:"Turn Claudinho off? It only comes back when you press BOOT (or RESET) on the board.",feito:"Done!",negado:"Not allowed from the panel",
 humores:["happy","question","sleepy","angry","done","thinking","excited","worried","startled","dizzy"],
 caras:["sleeping","neutral","thinking","working","waiting for you","done","happy","excited","worried","startled","dizzy","tired","sweating","angry","sad","suspicious"]};
for(const k of["t0","t1","t2","t3","t4","t5","t6","t7","l1","l2","l3","l4","l5"])document.getElementById(k).textContent=T[k];
const HUM=["feliz","pergunta","sono","bravo","pronto","pensando","empolgado","preocupado","susto","zonzo"];
for(const[id,k]of[["btest","testar"],["bcor","aplicar"],["bsalva","salvar"],["bpal","paleta"],["bcons","consumo"],["bimp","impressora"],["bvelha","velha"],["bgen","genius"],["bdemo","demo"],["breini","reiniciar"],["bdesl","desligar"]])document.getElementById(id).textContent=T[k];
document.getElementById("humor").innerHTML=HUM.map((h,i)=>`<option value="${h}">${T.humores[i]}</option>`).join("");
const K=new URLSearchParams(location.hash.slice(1)).get("k")||"";
const $=i=>document.getElementById(i);
const dur=s=>{const d=Math.floor(s/86400),h=Math.floor(s%86400/3600),m=Math.floor(s%3600/60);return d?d+"d "+h+"h":h?h+"h "+String(m).padStart(2,"0")+"min":m+"min"};
const cor=p=>p>=90?"var(--cr)":p>=75?"var(--al)":"var(--ok)";
function barra(nome,p,ren,agora){const ok=p>=0;return `<div class="l"><span>${nome}</span><b>${ok?p+"%":"--"}</b></div>
<div class="b"><i style="width:${ok?Math.min(p,100):0}%;background:${cor(p)}"></i></div>${ren>agora?`<div class="m">${T.ren+dur(ren-agora)}</div>`:""}`}
function esc(s){return String(s).replace(/[&<>"]/g,c=>({"&":"&amp;","<":"&lt;",">":"&gt;",'"':"&quot;"})[c])}
async function busca(){
 try{
  const r=await fetch("painel.json?k="+encodeURIComponent(K),{cache:"no-store"});
  if(r.status==401){$("erro").textContent=T.chave;$("erro").style.display="block";return}
  const d=await r.json();$("erro").style.display="none";ajustes(d);
  $("sub").textContent=`${d.placa} · firmware ${d.versao}`;
  $("cara").textContent=T.caras[d.cara]||"-";$("sess").textContent=`${d.n} ${T.sess}`;
  $("wifi").textContent=d.rssi+" dBm";$("ip").textContent=d.ip;
  $("liga").textContent=dur(d.ligado);$("mem").textContent=Math.round(d.heap/1024)+" "+T.livre;
  if(d.bat_pct>=0){$("cbat").style.display="";$("bat").textContent=d.bat_pct+"%";$("batm").textContent=(d.bat_carga?T.carga:T.bat)+" · "+d.bat_mv+" mV"}
  if(d.bambu){$("cimp").style.display="";$("imp").textContent=d.best||"-";$("impm").textContent=d.bpct>=0?d.bpct+"%":""}
  $("usos").innerHTML=barra(T.h5,d.h5,d.h5r,d.now)+barra(T.d7,d.d7,d.d7r,d.now)+barra(T.ctx,d.ctx,0,d.now)+(d.mod?`<div class="m" style="margin-top:10px">${esc(d.mod)}</div>`:"");
  $("hist").innerHTML=d.eventos.slice().reverse().map(e=>`<tr><td>${e.t?new Date(e.t*1000).toLocaleTimeString([], {hour:"2-digit",minute:"2-digit",second:"2-digit"}):"-"}</td><td>${esc(e.ev)}</td><td>${esc(T.caras[e.cara]||"")}</td></tr>`).join("");
 }catch(e){$("erro").textContent=T.sem;$("erro").style.display="block"}
}
let est={};
function aviso(t){const a=$("aviso");a.textContent=t;a.style.display="block";clearTimeout(aviso.x);aviso.x=setTimeout(()=>a.style.display="none",1800)}
async function manda(o){
 try{const r=await fetch("painel/cmd?k="+encodeURIComponent(K),{method:"POST",body:JSON.stringify(o)});
  aviso(r.ok?T.feito:r.status==403?T.negado:r.status==401?T.chave:"erro "+r.status);setTimeout(busca,400)}catch(e){aviso(T.sem)}
}
const hex=c=>"#"+[((c>>11)&31)*255/31,((c>>5)&63)*255/63,(c&31)*255/31].map(v=>Math.round(v).toString(16).padStart(2,"0")).join("");
const rgb=h=>[1,3,5].map(i=>parseInt(h.substr(i,2),16));
let corMexida=false,volMexido=false;
$("bsom").onclick=()=>manda({som:est.som?"desliga":"liga"});
$("vol").oninput=()=>{volMexido=true;$("vvol").textContent=$("vol").value+"%"};
$("vol").onchange=()=>{manda({som:"volume",volume:+$("vol").value});volMexido=false};
$("btest").onclick=()=>manda({som:$("humor").value});
$("cor").oninput=()=>corMexida=true;
$("bcor").onclick=()=>manda({cor:rgb($("cor").value)});
$("bsalva").onclick=()=>{manda({cor:rgb($("cor").value),salvar:true});corMexida=false};
$("bpal").onclick=()=>manda({paleta:true});
$("idi").onchange=()=>manda({idioma:$("idi").value});
$("bcons").onclick=()=>manda({consumo:20});
$("bimp").onclick=()=>manda({painel:true});
$("bvelha").onclick=()=>manda({velha:true});
$("bgen").onclick=()=>manda({genius:true});
$("bdemo").onclick=()=>manda({demo:true});
$("breini").onclick=()=>manda({reiniciar:true});
$("bdesl").onclick=()=>{if(confirm(T.conf))manda({desligar:true})};
function ajustes(d){
 est=d;
 $("ajsom").style.display=d.som<0?"none":"";
 $("bsom").textContent=d.som?T.ligado:T.mudo;$("bsom").className=d.som?"on":"";
 if(!volMexido){$("vol").value=d.vol;$("vvol").textContent=d.vol+"%"}
 if(!corMexida)$("cor").value=hex(d.cor);
 if(document.activeElement!=$("idi"))$("idi").value=d.idioma;
 $("bimp").style.display=d.bambu?"":"none";
 $("bdesl").style.display=d.bat_pct>=0?"":"none";
}
busca();setInterval(busca,3000);
</script></body></html>)HTML";
