# openre4 — Resident Evil 4 (GameCube) rodando nativo no PC

Porte nativo de **Resident Evil 4** (build de depuração `G4BE08` do GameCube, "Nov 25 2004") para PC moderno, a partir da descompilação completa em C/C++ — **sem emulador**. O objetivo final é um executável multiplataforma com **multiplayer cooperativo** via servidor dedicado.

> Base de descompilação: [adonis-singh/re4](https://github.com/adonis-singh/re4) (mantido como `upstream`).
> Camada nativa (janela, vídeo, input, DVD): [Aurora](https://github.com/encounter/aurora) via `vendor/aurora` (SDL3 + Vulkan/Dawn).

---

## Status atual (out/2026 — verificado no repo)

| Frente | Estado |
| :--- | :--- |
| Build nativo Linux x86_64 (`build/openre4`, ~33 MB) | ✅ Compila limpo (`ninja -C build`) |
| Janela SDL3 + backend Vulkan (Intel Iris Xe / Mesa) | ✅ Abre, 60 FPS, fecha/redimensiona |
| Montagem do disco original (`G4BE08`) | ✅ `GameID: G4BE` via Aurora DVD |
| Inicialização (`systemStartInit`, `Title_task`) | ✅ Roda |
| Vila `r120` → gameplay (`Rno0=3`) | 🔄 `gameRoomInit COMPLETE`; frames avançam e o 3D desenha, mas a câmera do cut (`r120s00`) vai para X≈-119000 → imagem distorcida (ver "Tarefas em andamento") |
| Transição para `r100` | 🔄 Leitura (`req=27 st1/r100.das`) destravada junto com o fix de fibras; falta validar após a câmera |
| Teclado/gamepad PC | ✅ Mapeado (tabela abaixo) |
| Servidor multiplayer em Rust | ❌ Não iniciado (planejado) |
| Modo Mercenaries co-op | ❌ Não iniciado (planejado) |
| Windows / macOS / Switch / Android | ❌ Não iniciado (planejado) |

Detalhes da arquitetura e das 5 fases: [`docs/ROADMAP.md`](docs/ROADMAP.md).

---

## Tarefas em andamento (handoff para agentes)

> Atualizado em out/2026. Fluxo atual: título → START → sala `r120` (Vila) → `gameRoomInit COMPLETE`
> → `Rno0 = 3` (gameplay ativo). O jogo roda estável (frames avançando, ~2–8 fps sob lavapipe),
> o filme `movie/opening.sfd` é pulado (sem backend de vídeo) e o 3D desenha — porém a imagem
> sai distorcida. **Estado de build:** `ninja -C build` limpo.

### Feito nesta fase (não regredir)
- [x] **Fibras/scheduler**: `TaskSchedulerMain` chamado de dentro da fibra do jogo (SCE tasks,
      `src/game/sce_sys.cpp:225`) fazia o `FiberYieldBack` devolver `s_currentThread = main`
      e o `TaskSleep` seguinte era pulado → GameTask rodava solto (~41k fps), main travava e o
      `Dvd.Watcher()` nunca rodava (stall do `req=27 st1/r100.das`). Correção em
      `src/platform/dolphin_shims.cpp`: `NativeFiberInfo::resumeThread`, `OSResumeThread` salva
      resumidor, `FiberYieldBack` restaura, `OSResumeThread(main)` no-op no nativo,
      `TaskExit`/`TaskChain` usam `OSExitThread`.
- [x] **`cPartsWk` em `src/game/trans.cpp`**: offsets eram PS2 (0xF4/0xF8); no x64
      `cParts::pList` = 0x108 e `lt_inv_mat` = 0x110 (`cUnit` = 0x18). Era o crash
      `C_MTXConcat(b=NULL)` no 1º frame 3D.
- [x] **Arrays indexados**: o shim 3-arg `GXSetArray` do Aurora registra 256 MB
      (`vendor/aurora/lib/dolphin/gx/GXGeometry.cpp:237`) → `bad_alloc`/abort no worker GX.
      Adicionado `GXSetArraySized` (`src/platform/gx_native.cpp`, declarado em `include/gx.h`)
      e usado em `trans.cpp`, `shadow.cpp`, `mirror.cpp`, `Espgen42/43/45.cpp` com tamanhos
      reais (`nVtx*stride` / `(W+1)*(H+1)*sizeof(Vec)`).
- [x] **`MemSignalHeap`** (`src/game/main_mem.cpp`): usava `(u32) heap_backup` (endereço
      nativo de 64 bits truncado) ao restaurar heap suspenso após o filme → SIGSEGV. Agora
      `hh[h] = bk[h]`.
- [x] **Skip do filme**: sem backend, `startApp()==0` → `finishMovie()`; a restauração de heap
      (`MemDestroyHeap(11)` + `Aram.DmaTransReq` + `MemSignalHeap`) agora funciona.
      Ver `src/game/sofdec.cpp` (`initSub`, `ThreadMove`, `finishMovie`).

### Tarefa ativa (bloqueio atual): câmera do cut EVD distorcida
- [ ] **Câmera do cutscene `r120s00` vai para X≈-119000** (a câmera de gameplay fica em
      ~(-500, 1765, -1190)) → geometria vira "fatias" gigantes na tela.
      Instrumentação nova (remover depois): `[CUTDBG]`/`[CUTRAW]` em `src/game/cam_motion.cpp`
      (`CameraMotion::move`), `[CAMDBG]`/`[PROJDBG]` em `src/game/main.cpp` (`bio4_frame`).
      Já verificado: projeção OK (`ZNEAR=100`, `ZFAR=1e6`, `m22≈-0.0001`, `m23≈-100.01`,
      fovy do cut 9.71° é plausível); tracks Y/Z plausíveis (~1400, ~-800), só o X explode.
      Linha do tempo do cut: frame 0 → X≈-118953; frame 120 → X≈-87531.
      **Próximo passo**: `[CUTRAW]` mostra pos/at crus e a translação de `m_p_base_mat`
      (`MatCamOya` do packet 9 "cam0000", `src/game/event.cpp:1145`) — decidir se o valor
      bruto já vem errado (formato/endian dos keys em `src/game/motion.cpp`:
      `UnpackMotionNative:132`, `HermiteInterpolation`/`Fcc_get_data_*`) ou se é a matriz base
      que está corrompida (construção em `ExePacket_Pos`: `RotMatrix`/`TransMatrix`).
- [ ] Depois: validar eventos/câmera do `r120s00`/`r120s01` e a transição para `st1/r100.das`
      (o stall do `req=27` era consequência do bug de fibras — leitura já completa em 1 spin).
- [ ] Olhar a cena 3D com a câmera correta (tela preta/distorção restante).

### Limpeza/entrega pendentes
- [ ] Remover instrumentação temporária: `[FDBG]`, `[CAMDBG]`, `[PROJDBG]` (`main.cpp`),
      `[CUTDBG]`, `[CUTRAW]` (`cam_motion.cpp`), `[GTASKDBG]` (`game.cpp`), `[ITASKDBG]`
      (`scheduler.cpp`), `[FIBERDBG]`+ring       (`dolphin_shims.cpp`), prints `[OpenRE4 Debug]` de `calcAddr` (`cam_ctrl.cpp`),
      `[WDBG]`, `[DMGDBG]`, `[CMDBG]`,
      `[EVTDBG]`, `[SATDBG]`, prints `ReadAreaData`, `gameRoomInit`, `DvdDumpState`.
- [ ] Rebuild limpo + commit + push; pedir teste no desktop real do usuário (fora do Xvfb/lavapipe).

### Como reproduzir/testar (Linux, Xvfb)
```sh
ninja -C build
# Xvfb :99 -screen 0 1400x1000x24 &
ps -eo pid,args | grep "build/openre4" | grep -v grep | awk '{print $1}' | xargs -r kill -9
setsid bash -c 'DISPLAY=:99 SDL_VIDEODRIVER=x11 \
  VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/lvp_icd.json \
  stdbuf -oL -eL ./build/openre4 > /tmp/run.log 2>&1; echo "EXIT:$?" > /tmp/run.exit' &
sleep 32; export DISPLAY=:99
xdotool mousemove 640 630 mousedown 1 sleep 1 mouseup 1      # START
sleep 6; xdotool keydown space; sleep 1.2; xdotool keyup space  # titleExit
sleep 40
# screenshot (única forma instalada; xwd/import não existem):
timeout -k 2 10 ffmpeg -loglevel error -f x11grab -video_size 1400x1000 \
  -i :99 -frames:v 1 -y /tmp/shot.png < /dev/null
```
Crash: o handler imprime `[OpenRE4 FATAL] sinal N @ pc=... addr=...` + frames `+0x...`;
decodificar com `addr2line -f -C -e build/openre4 <offset>`. Cuidado com `ffmpeg` x11grab
(sempre `timeout`) e com prints em loop (logs de GB).

---

## Como funciona (sem emulação)

O código do jogo chama as funções originais do SDK do GameCube (`GXBegin`, `PADRead`, `DVDRead`, …). O Aurora **reimplementa essas funções em C++ moderno**: cada chamada vira SDL3/Vulkan nativo e tudo é compilado junto num único binário (`openre4`). Zero tradução de binário em tempo de execução.

```
src/game/*.cpp (lógica RE4) ──┐
src/st1/*.cpp (salas Vila) ───┼──► openre4 (ELF x86_64 / .exe)
src/platform/*.cpp (shims) ───┘         ▲
vendor/aurora (GX→Vulkan, PAD→SDL, DVD) ─┘
```

---

## Pré-requisitos

* Linux x86_64 (testado em Pop!_OS 24.04, Mesa Vulkan 1.3), Python 3, CMake ≥ 3.25, Ninja, GCC/Clang com suporte a C++20
* **Sua própria imagem** dos discos de depuração do GameCube `G4BE08` (Disc 1 + Disc 2, `.iso`/`.gcm`) — **não incluída no repo** por motivos legais

## Build

```sh
git clone --recurse-submodules https://github.com/DevCoderMax/openre4.git
cd openre4

# 1. discos (forneça os seus)
cp re4_debug_disc1.gcm re4_debug_disc2.gcm orig/G4BE08/  # renomeie para re4_disc1.gcm / re4_disc2.gcm

# 2. extrair assets (1,6 GB em data/)
python3 tools/extract_all_assets.py data

# 3. compilar o executável nativo
cmake -S . -B build -G Ninja
ninja -C build
```

## Executar

```sh
./build/openre4               # título (pula logos por padrão)
./build/openre4 --village     # vai direto p/ a Vila, sala r100 (modo debug atual)
./build/openre4 --show-logos  # exibe as telas de logo originais
```

### Controles (mapeamento PC atual)

| Tecla | GameCube |
| :--- | :--- |
| `W A S D` | Analógico principal (mover Leon) |
| `I J K L` | C-Stick (câmera) |
| `ESPAÇO` / `ENTER` | `A` (confirma/ação) |
| `X` / `ESC` | `B` (cancela/corre) |
| `C` / `V` | `X` / `Y` |
| `Q` / `E` | Gatilhos `L` / `R` (mirar/atirar) |
| `TAB` | Gatilho `Z` (mapa) |
| Setas | D-Pad (menus) |
| `ENTER` | `START` |

---

## Estrutura

```
openre4/
├── src/platform/      # código do porte: main_native.cpp, dolphin_shims.cpp, em/wep_native.cpp
├── src/game/          # lógica do jogo (descompilação)
├── src/st1/           # salas do Estágio 1 (Vila/Pueblo)
├── src/em*, src/pl*, src/wep*/  # inimigos, personagens, armas
├── include/           # headers (structs reconstruídas, SDK)
├── vendor/aurora/     # HAL GameCube→PC (submódulo)
├── tools/             # scripts (extração, diff, build matching)
├── orig/G4BE08/       # SUAS imagens de disco (ignorado pelo git)
├── data/              # assets extraídos (ignorado pelo git)
├── docs/ROADMAP.md    # planejamento técnico + multiplayer
└── CMakeLists.txt     # build nativo (Ninja)
```

> `configure.py` / `config/G4BE08/` pertencem ao fluxo original de *byte-matching* com os compiladores históricos (ProDG/CodeWarrior). O build nativo usa apenas o `CMakeLists.txt`.

---

## Multiplayer (planejado, não implementado)

Servidor dedicado em Rust (UDP, 60 Hz) como autoridade de salas/IA; clientes enviam `PlayerSyncPacket` (posição, mira, arma, animação). MVP previsto: **Mercenaries co-op**, depois campanha. Ver [`docs/ROADMAP.md`](docs/ROADMAP.md) fases 3–5.

---

## Créditos e legal

* Descompilação base: projeto `re4` (comunidade, `upstream`) — nomes de funções da Capcom via `Bio4.sym`.
* HAL nativa: projeto Aurora (Luke Street / encounter) + lições da cena (Dolphin, Metaforce, Dusklight).
* **Você precisa possuir o jogo original.** Nenhum asset, ISO ou executável da Capcom acompanha este repositório (`orig/`, `data/` e `*.gcm/*.iso` são ignorados pelo git).
* O código reconstruído do jogo e do SDK pertence a Capcom, Nintendo e CRI Middleware e é publicado **para pesquisa e preservação**. Scripts, ferramentas e docs próprios do projeto: CC0 (ver `LICENSE`).
