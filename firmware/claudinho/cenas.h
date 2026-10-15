// Gerado por scripts/gera-cenas.py. Nao editar: mude os roteiros la.
#pragma once

static const char* const ARQ_0[] = {
  "\x04" "// acorda o Claudinho",
  "\x07" "void " "\x02" "acorda" "\x06" "() {",
  "\x07" "  int " "\x06" "n = " "\x05" "42" "\x06" ";",
  "\x01" "  if " "\x06" "(n > " "\x05" "0" "\x06" ") {",
  "\x06" "    tela." "\x02" "pisca" "\x06" "(" "\x05" "3" "\x06" ");",
  "\x02" "    fala" "\x06" "(" "\x03" "'oi!'" "\x06" ");",
  "\x06" "  }",
  "\x01" "  for " "\x06" "(" "\x07" "int " "\x06" "i = " "\x05" "0" "\x06" "; i < n; i++)",
  "\x06" "    olho[i] = " "\x01" "true" "\x06" ";",
  "\x06" "}",
  "",
  "\x04" "// quanto custa? nada.",
  "\x07" "int " "\x02" "tokens" "\x06" "() {",
  "\x01" "  return " "\x05" "0" "\x06" ";",
  "\x06" "}",
  "",
  "\x07" "bool " "\x02" "feliz" "\x06" "(" "\x07" "int " "\x06" "humor) {",
  "\x01" "  while " "\x06" "(humor < " "\x05" "10" "\x06" ")",
  "\x06" "    humor += " "\x02" "cafe" "\x06" "();",
  "\x01" "  return true" "\x06" ";",
  "\x06" "}",
  "",
  "\x04" "/* TODO: dominar o mundo */",
  "\x07" "void " "\x02" "loop" "\x06" "() {",
  "\x02" "  acorda" "\x06" "();",
  "\x02" "  delay" "\x06" "(" "\x05" "10" "\x06" ");",
  "\x06" "}",
  "",
};
static const char* const ARQ_1[] = {
  "\x01" "import " "\x06" "time",
  "\x01" "from " "\x06" "jwt " "\x01" "import " "\x06" "decode",
  "",
  "\x06" "TTL = " "\x05" "900  " "\x04" "# 15 min",
  "",
  "\x01" "def " "\x02" "check" "\x06" "(tok: " "\x07" "str" "\x06" ") -> " "\x07" "dict" "\x06" ":",
  "\x01" "    try" "\x06" ":",
  "\x06" "        c = " "\x02" "decode" "\x06" "(tok, KEY)",
  "\x01" "    except " "\x07" "Expired" "\x06" ":",
  "\x01" "        raise " "\x07" "AuthErr" "\x06" "(" "\x03" "\"expired\"" "\x06" ")",
  "\x01" "    if " "\x06" "c[" "\x03" "\"iat\"" "\x06" "] + TTL < " "\x02" "now" "\x06" "():",
  "\x01" "        raise " "\x07" "AuthErr" "\x06" "(" "\x03" "\"stale\"" "\x06" ")",
  "\x01" "    return " "\x06" "c",
  "",
  "\x01" "def " "\x02" "refresh" "\x06" "(user):",
  "\x06" "    tok = " "\x02" "sign" "\x06" "({" "\x03" "\"sub\"" "\x06" ": user.id})",
  "\x06" "    cache." "\x02" "set" "\x06" "(user.id, tok, TTL)",
  "\x01" "    return " "\x06" "tok",
  "",
};
static const char* const ARQ_2[] = {
  "\x01" "import " "\x06" "{ useState } " "\x01" "from " "\x03" "\"react\"",
  "",
  "\x01" "export function " "\x02" "useCart" "\x06" "() {",
  "\x01" "  const " "\x06" "[items, setItems] =",
  "\x02" "    useState" "\x06" "<" "\x07" "Item" "\x06" "[]>([]);",
  "",
  "\x01" "  const " "\x02" "add" "\x06" " = (it: " "\x07" "Item" "\x06" ") =>",
  "\x02" "    setItems" "\x06" "(xs => [...xs, it]);",
  "",
  "\x01" "  const " "\x06" "total = items." "\x02" "reduce" "\x06" "(",
  "\x06" "    (s, x) => s + x.price, " "\x05" "0" "\x06" ");",
  "",
  "\x01" "  return " "\x06" "{ items, add, total };",
  "\x06" "}",
  "",
};
static const char* const ARQ_3[] = {
  "\x01" "package " "\x06" "ratelimit",
  "",
  "\x01" "import " "\x03" "\"time\"",
  "",
  "\x01" "type " "\x07" "Bucket " "\x01" "struct " "\x06" "{",
  "\x06" "  tokens " "\x07" "int",
  "\x06" "  last   " "\x07" "time.Time",
  "\x06" "}",
  "",
  "\x01" "func " "\x06" "(b *" "\x07" "Bucket" "\x06" ") " "\x02" "Take" "\x06" "() " "\x07" "bool " "\x06" "{",
  "\x06" "  b." "\x02" "refill" "\x06" "()",
  "\x01" "  if " "\x06" "b.tokens == " "\x05" "0 " "\x06" "{",
  "\x01" "    return false",
  "\x06" "  }",
  "\x06" "  b.tokens--",
  "\x01" "  return true",
  "\x06" "}",
  "",
  "\x04" "// one token per second",
  "\x01" "func " "\x06" "(b *" "\x07" "Bucket" "\x06" ") " "\x02" "refill" "\x06" "() {",
  "\x06" "  dt := time." "\x02" "Since" "\x06" "(b.last)",
  "\x06" "  b.tokens += " "\x02" "int" "\x06" "(dt." "\x02" "Seconds" "\x06" "())",
  "\x06" "  b.last = time." "\x02" "Now" "\x06" "()",
  "\x06" "}",
  "",
};
static const char* const ARQ_4[] = {
  "\x04" "-- users and their plans",
  "\x01" "CREATE TABLE " "\x06" "users (",
  "\x06" "  id    " "\x07" "SERIAL " "\x01" "PRIMARY KEY" "\x06" ",",
  "\x06" "  email " "\x07" "TEXT " "\x01" "UNIQUE NOT NULL" "\x06" ",",
  "\x06" "  plan  " "\x07" "TEXT " "\x01" "DEFAULT " "\x03" "'pro'",
  "\x06" ");",
  "",
  "\x01" "CREATE INDEX " "\x06" "idx_email",
  "\x01" "  ON " "\x06" "users (email);",
  "",
  "\x01" "SELECT " "\x06" "plan, " "\x02" "count" "\x06" "(*)",
  "\x01" "  FROM " "\x06" "users",
  "\x01" " GROUP BY " "\x06" "plan",
  "\x01" " ORDER BY " "\x05" "2 " "\x01" "DESC" "\x06" ";",
  "",
};
struct Arquivo { const char* nome; const char* const* l; int n; };
static const Arquivo ARQUIVOS[] = {
  { "claudinho.ino", ARQ_0, sizeof ARQ_0 / sizeof ARQ_0[0] },
  { "auth.py", ARQ_1, sizeof ARQ_1 / sizeof ARQ_1[0] },
  { "useCart.ts", ARQ_2, sizeof ARQ_2 / sizeof ARQ_2[0] },
  { "bucket.go", ARQ_3, sizeof ARQ_3 / sizeof ARQ_3[0] },
  { "schema.sql", ARQ_4, sizeof ARQ_4 / sizeof ARQ_4[0] },
};
static const int N_ARQUIVOS = sizeof ARQUIVOS / sizeof ARQUIVOS[0];

static const char* const SES_0[] = { "$git status", "On branch main", "Changes not staged for commit:", "  modified:   src/api.ts", "  modified:   src/api.test.ts", "$git add -A", "$git commit -m \"fix: retry on 429\"", "[main 3f9c2ab] fix: retry on 429", " 2 files changed, 18 insertions(+)", "$git push", "To github.com:you/app.git", "   9be2c10..3f9c2ab  main -> main" };
static const char* const SES_1[] = { "$npm test", "> app@2.3.1 test", "> vitest run", "", " PASS  src/cart.test.ts (12)", " PASS  src/auth.test.ts (8)", " FAIL  src/api.test.ts (1/4)", "   expected 200, received 429", "Tests  1 failed | 23 passed", "$npm test -- api", " PASS  src/api.test.ts (4)", "Tests  4 passed (4)" };
static const char* const SES_2[] = { "$pytest -q", ".........F......", "FAILED test_auth.py::test_stale", "1 failed, 15 passed in 0.84s", "$pytest -q -k stale", ".", "1 passed in 0.11s" };
static const char* const SES_3[] = { "$npm run build", "vite v5.4.8 building for prod...", "transforming...", "412 modules transformed.", "dist/index.html       0.46 kB", "dist/index.js       142.31 kB", "built in 3.21s" };
static const char* const SES_4[] = { "$docker compose up -d", " Network app_default  Created", " Container app-db-1   Started", " Container app-api-1  Started", "$curl -s localhost:8000/health", "{\"status\":\"ok\",\"db\":\"up\"}" };
static const char* const SES_5[] = { "$cargo build --release", "   Compiling serde v1.0.210", "   Compiling tokio v1.40.0", "   Compiling app v0.4.0", "    Finished release in 23.4s", "$./target/release/app -V", "app 0.4.0" };
static const char* const SES_6[] = { "$grep -rn TODO src | wc -l", "17", "$git log --oneline -3", "3f9c2ab fix: retry on 429", "a1d07e4 feat: dark mode", "9be2c10 chore: bump deps" };
static const char* const SES_7[] = { "$pio run -t upload", "Building .pio/build/esp32", "RAM:   [==        ]  20.6%", "Flash: [========  ]  82.5%", "Uploading 1622496 bytes", "Writing at 0x0019c000 (100 %)", "Hard resetting via RTS pin..." };
static const char* const SES_8[] = { "$python -m venv .venv", "$pip install -r requirements.txt", "Collecting fastapi==0.115.0", "Collecting uvicorn==0.30.6", "Successfully installed 14 packages", "$uvicorn main:app --reload", "INFO:  Uvicorn running on :8000" };
struct Sessao { const char* const* l; int n; };
static const Sessao SESSOES[] = {
  { SES_0, sizeof SES_0 / sizeof SES_0[0] },
  { SES_1, sizeof SES_1 / sizeof SES_1[0] },
  { SES_2, sizeof SES_2 / sizeof SES_2[0] },
  { SES_3, sizeof SES_3 / sizeof SES_3[0] },
  { SES_4, sizeof SES_4 / sizeof SES_4[0] },
  { SES_5, sizeof SES_5 / sizeof SES_5[0] },
  { SES_6, sizeof SES_6 / sizeof SES_6[0] },
  { SES_7, sizeof SES_7 / sizeof SES_7[0] },
  { SES_8, sizeof SES_8 / sizeof SES_8[0] },
};
static const int N_SESSOES = sizeof SESSOES / sizeof SESSOES[0];
