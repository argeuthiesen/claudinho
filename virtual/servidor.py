#!/usr/bin/env python3
"""Claudinho virtual: o mascote numa aba do navegador, sem placa.

Faz o papel da placa para os scripts do plugin: recebe os mesmos POST /evento
(hooks) e /estado (status line), com o mesmo segredo, e repassa para a pagina
(claudinho.html) por Server-Sent Events. So escuta em 127.0.0.1: nada sai do
computador e nada de fora entra. Para ler os eventos, a pagina precisa da
chave de leitura, que so o dono recebe (no endereco que o claudinho.sh abre).

Uso: python3 servidor.py PORTA   com tres linhas na entrada padrao:
     segredo (o dos hooks), chave de leitura (da pagina) e um "oi" que o
     /mini.json devolve, para o claudinho.sh saber que subiu este servidor.
     Normalmente quem chama e o "claudinho.sh virtual".

Claudinho virtual: the mascot in a browser tab, no board needed. It stands in
for the board (same /evento and /estado, same secret) and forwards everything
to the page over Server-Sent Events. Listens on 127.0.0.1 only; reading the
events takes a read key that only the owner gets.
"""
import hmac, json, queue, sys, threading, time
from http.server import ThreadingHTTPServer, BaseHTTPRequestHandler
from pathlib import Path

AQUI = Path(__file__).resolve().parent
ARQUIVOS = {"/": ("claudinho.html", "text/html; charset=utf-8"),
            "/modelo/corpo.stl": ("modelo/corpo.stl", "model/stl"),
            "/modelo/moldura.stl": ("modelo/moldura.stl", "model/stl"),
            "/modelo/base.stl": ("modelo/base.stl", "model/stl")}
MAX_CORPO = 8192
MAX_ABAS = 8                      # abas abertas ao mesmo tempo
MAX_FILA = 200                    # eventos esperando uma aba lenta; passou disso, ela cai
SESSAO_TTL = 30 * 60              # o mesmo da placa

segredo = chave = oi = ""
ouvintes = []                     # uma fila por aba aberta
trava = threading.Lock()          # protege ouvintes, sessoes e ultimo_estado
ultimo_estado = None              # para a aba que abre depois ja mostrar o consumo
sessoes = {}                      # sessao -> ultima vez vista


def conta_sessoes():              # chamar com a trava
    agora = time.time()
    for s in [s for s, t in sessoes.items() if agora - t > SESSAO_TTL]:
        sessoes.pop(s, None)
    return len(sessoes)


def manda(tipo, dados):           # chamar com a trava
    for q in list(ouvintes):
        try:
            q.put_nowait((tipo, dados))
        except queue.Full:
            q.cheia = True        # a aba nao esta lendo: o laco dela encerra


class Pedido(BaseHTTPRequestHandler):
    server_version = "Claudinho"
    sys_version = ""
    timeout = 20                  # conexao parada nao segura uma thread para sempre

    def log_message(self, *a):    # mudo: nada de log no terminal
        pass

    def host_ok(self):            # contra "DNS rebinding": so aceita o proprio endereco
        h = (self.headers.get("Host") or "").rsplit(":", 1)[0]
        return h in ("127.0.0.1", "localhost")

    def responde(self, codigo, corpo=b"ok\n", tipo="text/plain"):
        self.send_response(codigo)
        self.send_header("Content-Type", tipo)
        self.send_header("Content-Length", str(len(corpo)))
        self.send_header("Cache-Control", "no-store")
        self.send_header("X-Content-Type-Options", "nosniff")
        self.end_headers()
        self.wfile.write(corpo)

    def do_GET(self):
        if not self.host_ok():
            return self.responde(403, b"host\n")
        caminho, _, consulta = self.path.partition("?")
        if caminho == "/eventos":
            k = dict(p.split("=", 1) for p in consulta.split("&") if "=" in p).get("k", "")
            if not hmac.compare_digest(k, chave):
                return self.responde(401, b"chave invalida\n")
            return self.eventos()
        if caminho == "/mini.json":
            with trava:
                n = conta_sessoes()
            return self.responde(200, json.dumps({"versao": "virtual", "placa": "virtual", "n": n, "oi": oi}).encode() + b"\n",
                                 "application/json")
        if caminho in ARQUIVOS:
            nome, tipo = ARQUIVOS[caminho]
            try:
                return self.responde(200, (AQUI / nome).read_bytes(), tipo)
            except OSError:
                return self.responde(404, b"nao achei\n")
        self.responde(404, b"nao achei\n")

    def do_POST(self):
        if not self.host_ok():
            return self.responde(403, b"host\n")
        if not hmac.compare_digest(self.headers.get("Authorization") or "", f"Bearer {segredo}"):
            return self.responde(401, b"segredo invalido\n")
        try:
            tam = int(self.headers.get("Content-Length") or 0)
        except ValueError:
            tam = -1
        if not 0 <= tam <= MAX_CORPO:
            return self.responde(413, b"tamanho invalido\n")
        try:
            doc = json.loads(self.rfile.read(tam) or b"{}")
            if not isinstance(doc, dict):
                raise ValueError
        except ValueError:
            return self.responde(400, b"json invalido\n")
        caminho = self.path.split("?")[0]
        s = str(doc.get("sessao") or "")[:12]
        if caminho == "/evento":
            ev = {k: str(doc.get(k) or "")[:24] for k in ("tipo", "humor", "acao")}
            with trava:
                if ev["tipo"] == "fim":
                    sessoes.pop(s, None)
                elif s:
                    sessoes[s] = time.time()
                ev["n"] = conta_sessoes()
                manda("evento", ev)
            return self.responde(200)
        if caminho == "/estado":
            global ultimo_estado
            with trava:
                if s:
                    sessoes[s] = time.time()
                doc["n"] = conta_sessoes()
                doc["visto"] = int(time.time())    # quando chegou: a aba nova sabe se e velho
                ultimo_estado = doc
                manda("estado", doc)
            return self.responde(200)
        if caminho == "/cmd":         # claudinho.sh: os comandos da placa nao fazem sentido aqui
            return self.responde(200)
        self.responde(404, b"nao achei\n")

    def eventos(self):
        q = queue.Queue(MAX_FILA)
        q.cheia = False
        with trava:
            if len(ouvintes) >= MAX_ABAS:
                return self.responde(503, b"abas demais\n")
            ouvintes.append(q)
            if ultimo_estado:         # o consumo de agora, com as sessoes recontadas
                q.put_nowait(("estado", dict(ultimo_estado, n=conta_sessoes())))
        try:
            self.send_response(200)
            self.send_header("Content-Type", "text/event-stream")
            self.send_header("Cache-Control", "no-store")
            self.end_headers()
            while not q.cheia:
                try:
                    tipo, dados = q.get(timeout=15)
                    self.wfile.write(f"event: {tipo}\ndata: {json.dumps(dados)}\n\n".encode())
                except queue.Empty:
                    self.wfile.write(b": oi\n\n")       # mantem a conexao viva
                self.wfile.flush()
        except OSError:
            pass
        finally:
            with trava:
                ouvintes.remove(q)


def main():
    global segredo, chave, oi
    porta = int(sys.argv[1]) if len(sys.argv) > 1 else 8737
    segredo, chave, oi = (sys.stdin.readline().strip() for _ in range(3))
    if not (segredo and chave and oi):
        sys.exit("faltam segredo, chave e oi (entrada padrao, uma linha cada)")
    srv = ThreadingHTTPServer(("127.0.0.1", porta), Pedido)
    srv.daemon_threads = True
    srv.serve_forever()


if __name__ == "__main__":
    main()
