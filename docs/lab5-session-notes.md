# Lab 5 — notas de sessão (tutor)

Última atualização: 2026-11-09. Leia este arquivo **antes** de qualquer coisa ao
retomar, em qualquer máquina. Substitui `docs/lab4-session-notes.md` (histórico).

## Como trabalhamos (combinado com o aluno)

- **Dinâmica senior/junior por tickets** (`RT-NN`): eu passo escopo, critérios
  de aceite e entregáveis; o aluno implementa; eu reviso como um PR review.
- **Estilo de review preferido:** apontar cada defeito com `arquivo:linha`,
  sintoma e **por que** está errado (o conceito). **Não** montar o passo a passo
  da correção, a menos que ele peça a resposta explicitamente ("como eu
  corrijo?"). Ele rejeitou os dois extremos: passo a passo não pedido e
  review só com perguntas.
- Continua valendo o `CLAUDE.md`: **eu não escrevo código C** do repositório.
  Exceção já concedida: o script de gráficos `lab/plot_results.py` (escrito por mim).
- **Git no lab 4/5: mínimo.** Commit direto na `main` com `git commit -m`; eu
  dou os comandos exatos. Do **lab 6 em diante** ele quer o fluxo completo
  (branch, PR, mensagens escritas por ele).
- Ao dar um comando de shell composto, **explicar cada pedaço**.
- **Sempre verificar rodando** (build, execução, testes de corrida) em vez de
  confiar em "funciona".
- Erros recorrentes do aluno: não ler o diff antes do commit; testar binário
  velho ou na pasta errada; **sobrescrever arquivos de medição** (em 09/11 o
  `noload_*.dat` da linha de base foi regravado); copiar e colar blocos sem
  ajustar nomes/campos; números mágicos sem unidade; chamar para review sem
  rodar `make`; typos em nomes de função nas mensagens de erro.
- Regra combinada: só chamar para review com `make` sem erros e sem warnings.
- Convenção de mensagem de erro (dele): `"<processo>: <função>"` via `perror`
  — o nome da função tem que ser exato (buscável no `man`/`grep`).
- Comentários em português restantes: **aceitos por enquanto** (decisão dele
  em 09/11); voltar a cobrar antes da entrega final.

## Ambiente — duas máquinas

| | Notebook | **Máquina RT** (usada em 09/11) |
|---|---|---|
| Kernel | `6.14.0-37-generic` (`PREEMPT_DYNAMIC`) | `7.2.8 #2 SMP PREEMPT_RT` |
| CPUs | 16 | 12 |
| `ulimit -r` / `-l` | 0 → precisa de `sudo` | **99 / unlimited** → sem `sudo` |
| Shell | bash | **zsh** (ver testes de corrida) |
| `rt-tests` | não instalado | não instalado; `sudo` pede senha |

Máquina RT, detalhes:
- `pandas` instalado com `pip3 install --user --break-system-packages pandas`
  (em `~/.local`, junto com numpy/matplotlib).
- `plot_results.py` precisa de `MPLBACKEND=Agg` (backend GTK quebra por
  config do GNOME).
- RT throttling **desligado** (`sched_rt_runtime_us` = `sched_rt_period_us` =
  1000000): laço `SCHED_FIFO` que não dorme trava um core.
- Threads RT do sistema: `migration/N` e `rtkit` em 99, áudio do navegador 98,
  PipeWire 83–88, IRQs e `watchdogd` em 50, `ktimers`/`rcuc` em 1.
- `ps -axHo ...` do enunciado: o `H` lista **threads** (processo com 50
  threads aparece 50×); serve tanto para processos quanto para threads.

Build e execução (de `lab/`):
```
make -C ../lib && make
./main <prefix>                       # gera <prefix>_u.dat, _sim.dat, _sampling.dat (20 s)
MPLBACKEND=Agg python3 plot_results.py <noload_prefix> [<load_prefix>]
```
Para as medições, usar prefixos que digam o cenário (ex.: `fifo_noload`,
`fifo_load`) para não sobrescrever a linha de base.

## Estado do código (`lab/main.c`)

3 processos via `fork()`: **simulator** (pai, 30 ms) cria **gen_u** (50 ms) e
**sampling** (50 ms). Pipe A: gen_u → simulator (u). Pipe B: simulator →
sampling (y). Leitura dos dois pipes `O_NONBLOCK`.

Pronto e validado (até `7479387`):
- Cada processo só mantém os fds que usa; `wait(&status)` com `ECHILD`/`EINTR`,
  reporta filho com erro/sinal e sai ≠ 0; desligamento sem SIGPIPE.
- Cada tarefa grava seu arquivo (`t_us` + dados), nome via `snprintf` checado.
- **RT-09:** `CLOCK_MONOTONIC` + `clock_nanosleep(TIMER_ABSTIME)`, alvo
  `t0 + k·T` (`timespec_add_ns`, typo já corrigido), retry em `EINTR`.

**RT-10 (implementado e validado em 09/11 na máquina RT, falta commit):**
- Constantes `PRIO_SIM 90`, `PRIO_GEN_U 80`, `PRIO_SAMPLING 70` (RM; desempate
  gen_u > sampling pelo fluxo de dados).
- O pai vira `SCHED_FIFO` **antes** do `fork()` → sem permissão falha com um
  processo só (testado com `ulimit -r 0`: `exit=1`, sem órfãos, sem arquivos).
  Cada filho ajusta a própria prioridade depois do `fork()` (janela curta na
  prioridade do pai, só na inicialização).
- `mlockall(MCL_CURRENT | MCL_FUTURE)` em **cada** processo, antes do laço
  (lock não é herdado no `fork()`). `VmLck` = 3600 kB nos 3.
- Verificado: `make -B` sem warnings; `ps` mostra `FF` 90/80/70;
  400/667/400 linhas; corridas A e B e status de filho passam.
- **Antes do commit:** typos nas mensagens `main.c:60` (`set_setscheduler`) e
  `main.c:286` (`sched_setsheduler`). Comando combinado:
  `git add lab/main.c && git diff --cached` e
  `git commit -m "feat: run tasks as SCHED_FIFO with rate monotonic priorities and mlockall"`.
- `lab/figures/` não versionado: ele ainda vai decidir se entra ou vai para
  `.gitignore`.

Medição sem carga, máquina RT (período, std / pior desvio de T):

| Tarefa | `SCHED_OTHER` (09/11 manhã) | `SCHED_FIFO` 99/97/94 |
|---|---|---|
| gen_u | 0,088 / 0,886 ms | 0,033 / 0,472 ms |
| simulator | 0,030 / 0,266 ms | 0,029 / 0,467 ms |
| sampling | 0,041 / 0,419 ms | 0,034 / 0,434 ms |

(Com 90/80/70 ainda não foi medido. Para o relatório, refazer as duas linhas
com prefixos distintos.)

Pendências menores conhecidas (não bloqueiam):
- `10000000000` (troca de u em 10 s) ainda é número mágico — já apontado 3×.
- Mensagem de uso `"Miss the file name"`, sinais impressos como número
  (sugestão `strsignal`), mensagens/comentários em português (linhas ~59, 73,
  83, 91, 174, 373 e comentários), `pid_wait = 1` como truque do `EINTR`.
- Um filho morrer **no meio** da execução derruba os outros por SIGPIPE em
  cascata. Risco baixo para falha de `sched_setscheduler` no filho (reduzir
  prioridade dentro de `SCHED_FIFO` não falha por permissão).
- Relatório: gen_u/sampling ficam abaixo do PipeWire (83–88); justificar ou
  medir com áudio/navegador fechados.

## Roteiro do lab 5 (`docs/lab5.pdf`, entrega era 27/10/2026)

| Ticket | O quê | Onde testar | Status |
|---|---|---|---|
| RT-09 | temporização absoluta (`clock_nanosleep`) | — | ✅ feito |
| RT-10 | `SCHED_FIFO` + prioridades RM + `mlockall` | máquina RT | ✅ validado, falta commit |
| **RT-11** | gravação em disco numa tarefa **não** tempo real | máquina RT | **← próximo** |
| RT-12 | tarefa não-RT que mostra y e u na tela | máquina RT | pendente |
| RT-13 | medições sem/com carga (`hackbench`), `ps -axHo pid,policy,ni,pri,rtprio,pcpu,psr,cmd`, relatório | máquina RT | pendente |

**Decisão processos × threads (09/11): ficar com processos + pipes.** O aluno
não conseguiu falar com o professor e delegou a recomendação; aceitou trocar
no lab 6 se o professor discordar. Motivos: o enunciado pede para *portar* o
programa do lab 4 (que já é com processos); o `ps -axHo` do enunciado funciona
para os dois desenhos; o código atual está validado e com testes de regressão.
Threads (memória compartilhada + mutex com herança de prioridade) ficam como
conteúdo provável de labs seguintes.

Ponto de desenho a levantar **como pergunta** no ticket RT-11 (não dar a
resposta): o que acontece com uma tarefa RT que escreve num pipe cujo leitor é
uma tarefa não-RT atrasada (pipe cheio → `write` bloqueante → inversão de
prioridade)? E o que fazer com dados quando não há espaço?

Ideias já combinadas para o lab 5:
- A métrica mais informativa é a **latência** L(k) = instante medido − alvo
  (sempre ≥ 0). Com timing absoluto, J(k) = L(k) − L(k−1), por isso J pode ser
  negativo e tem média ≈ 0 (bom parágrafo para o relatório).
- O simulator integra com `DT` fixo por ativação: ativação perdida = tempo
  simulado atrasado em relação ao real.
- Page fault em tarefa `SCHED_FIFO`: o atraso cai na tarefa com deadline e ela
  pode ficar esperando E/S de disco (não-RT) — parente da inversão de
  prioridade. `mlockall` é lento → só na inicialização.

## Testes de regressão úteis (rodar de `lab/`)

**Na máquina RT (zsh) rodar dentro de `bash -c '...'`**: o zsh sem controle de
jobs retoma (`SIGCONT`) sozinho um filho que ele vê parado, então a corrida B
não pausa nada (sintoma: `kill ... no such process` no `CONT`).

```
# corrida A: gen_u atrasa no fim (pai deve continuar ok)
( ./main raceA; echo "real exit=$?" ) & sleep 1; P=$(pgrep -o -x main); G=$(pgrep -P $P | sort -n | head -1); sleep 18.5; kill -STOP $G; sleep 2; kill -CONT $G; wait; tail -1 raceA_u.dat

# corrida B: simulator atrasa no fim (não pode morrer por SIGPIPE)
( ./main raceB; echo "real exit=$?" ) & sleep 1; P=$(pgrep -o -x main); sleep 18.5; kill -STOP $P; sleep 2; kill -CONT $P; wait; tail -1 raceB_sim.dat

# status de filho: matar o gen_u deve dar mensagem + exit 1, sem órfãos
( ./main t; echo "real exit=$?" ) & sleep 2; P=$(pgrep -o -x main); kill -TERM $(pgrep -P $P | sort -n | head -1); wait; ps -C main

# sem permissão de RT: deve falhar logo, exit 1, sem órfãos e sem arquivos
( ulimit -r 0; ./main failtest; echo "exit=$?" ); ps -C main; ls failtest_*
```
Esperado: `real exit=0` e última linha completa nas corridas (≈ 21,5 s com a
pausa); no terceiro, `child <pid> killed by signal`, `real exit=1` e `ps`
vazio; no quarto, `Operation not permitted`, `exit=1`, nada sobrando.
