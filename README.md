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
| Sala da Vila `r100` (`--village`) | 🔄 Carregando — `gameRoomInit: Camera completed`, `st1/r100_*.dat` lidos; geometria ainda não renderizada |
| Teclado/gamepad PC | ✅ Mapeado (tabela abaixo) |
| Servidor multiplayer em Rust | ❌ Não iniciado (planejado) |
| Modo Mercenaries co-op | ❌ Não iniciado (planejado) |
| Windows / macOS / Switch / Android | ❌ Não iniciado (planejado) |

Detalhes da arquitetura e das 5 fases: [`docs/ROADMAP.md`](docs/ROADMAP.md).

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
