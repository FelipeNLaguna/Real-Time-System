# Lab 5 — notas de sessão (tutor)

Última atualização: 2026-11-08. Leia este arquivo **antes** de qualquer coisa ao
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
  velho ou na pasta errada; copiar e colar blocos sem ajustar nomes/campos;
  números mágicos sem unidade; chamar para review sem rodar `make`.
- Regra combinada: só chamar para review com `make` sem erros e sem warnings.

## Ambiente

- Esta máquina (notebook): kernel genérico `6.14.0-37-generic`
  (`PREEMPT_DYNAMIC`, **não** RT), 16 CPUs, `ulimit -r` = 0 → `SCHED_FIFO` só
  com `sudo`. `hackbench`/`cyclictest` **não instalados** (`sudo apt install rt-tests`).
- Existe **outra máquina com PREEMPT_RT**: medições finais do lab 5 vão lá.
  Não compilar kernel.
- `gh` 2.46 (Ubuntu) tem bug no `gh pr edit` (erro `projectCards`); contorno:
  `gh api -X PATCH repos/FelipeNLaguna/Real-Time-System/pulls/<n> -f title=... -F body=@arquivo`.

Build e execução (de `lab/`):
```
make -C ../lib && make
./main <prefix>                       # gera <prefix>_u.dat, _sim.dat, _sampling.dat (20 s)
python3 plot_results.py noload load   # tabela + figuras em lab/figures/
```

## Estado do código (`lab/main.c`, commit `7479387`)

3 processos via `fork()`: **simulator** (pai, 30 ms) cria **gen_u** (50 ms) e
**sampling** (50 ms). Pipe A: gen_u → simulator (u). Pipe B: simulator →
sampling (y). Leitura dos dois pipes `O_NONBLOCK`.

Pronto e validado:
- Cada processo só mantém os fds que usa (RT-04, PR #1).
- Simulator espera os filhos com `wait(&status)`: `ECHILD` = fim normal,
  `EINTR` = repete, reporta filho com código ≠ 0 ou morto por sinal e sai ≠ 0.
- Desligamento sem SIGPIPE: simulator fecha a leitura do A **depois** do `wait`;
  sampling, depois do laço, drena o pipe B até **EOF** antes de fechar a leitura.
- Cada tarefa grava seu arquivo: `t_us` (µs desde t0) + dados. Nome =
  `argv[1]` + sufixo via `snprintf`, com checagem de truncamento.
- **RT-09 (temporização absoluta):** `CLOCK_MONOTONIC` +
  `clock_nanosleep(TIMER_ABSTIME)` com alvo `t0 + k·T` (`timespec_add_nss`), retry
  em `EINTR` pelo valor de retorno. Laço decide pelo **alvo**, não pela medição.
  Validado: 400/667/400 linhas, nenhuma ≥ 20 s, J médio ≈ 0
  (std ≈ 0,07–0,08 ms, J entre −0,43 e +0,31 ms, kernel genérico, sem carga).

Pendências menores conhecidas (não bloqueiam):
- `10000000000` (troca de u em 10 s) ainda é número mágico — já apontado 3×.
- `timespec_add_nss` (typo), mensagem de uso `"Miss the file name"`,
  sinais impressos como número (sugestão `strsignal`), comentários em português
  restantes, `pid_wait = 1` como truque do `EINTR`.
- Um filho morrer **no meio** da execução derruba os outros por SIGPIPE em
  cascata (fora do escopo do lab 4; reavaliar no lab 5).

## Roteiro do lab 5 (`docs/lab5.pdf`)

| Ticket | O quê | Onde testar | Status |
|---|---|---|---|
| RT-09 | temporização absoluta (`clock_nanosleep`) | aqui | ✅ feito |
| **RT-10** | `SCHED_FIFO` + prioridades (Rate Monotonic) + `mlockall` | aqui com `sudo` | **← próximo** |
| RT-11 | gravação em disco numa tarefa **não** tempo real | aqui | pendente |
| RT-12 | tarefa não-RT que mostra y e u na tela | aqui | pendente |
| RT-13 | medições sem/com carga, `ps -axHo pid,policy,ni,pri,rtprio,pcpu,psr,cmd`, relatório | máquina RT | pendente |

**Decisão em aberto (precisa antes do RT-11):** processos (como hoje, com
pipes) ou threads (`pthread`, memória compartilhada + mutex com herança de
prioridade)? O `-H` no `ps` do enunciado sugere threads. Aluno ia confirmar
com o professor/colegas.

Ideias já combinadas para o lab 5:
- A métrica mais informativa é a **latência** L(k) = instante medido − alvo
  (sempre ≥ 0). Com timing absoluto, J(k) = L(k) − L(k−1), por isso J pode ser
  negativo e tem média ≈ 0 (bom parágrafo para o relatório).
- O simulator integra com `DT` fixo por ativação: ativação perdida = tempo
  simulado atrasado em relação ao real.

## Testes de regressão úteis (rodar de `lab/`)

```
# corrida A: gen_u atrasa no fim (pai deve continuar ok)
( ./main raceA; echo "real exit=$?" ) & sleep 1; P=$(pgrep -o -x main); G=$(pgrep -P $P | sort -n | head -1); sleep 18.5; kill -STOP $G; sleep 2; kill -CONT $G; wait; tail -1 raceA_u.dat

# corrida B: simulator atrasa no fim (não pode morrer por SIGPIPE)
( ./main raceB; echo "real exit=$?" ) & sleep 1; P=$(pgrep -o -x main); sleep 18.5; kill -STOP $P; sleep 2; kill -CONT $P; wait; tail -1 raceB_sim.dat

# status de filho: matar o gen_u deve dar mensagem + exit 1, sem órfãos
( ./main t; echo "real exit=$?" ) & sleep 2; P=$(pgrep -o -x main); kill -TERM $(pgrep -P $P | sort -n | head -1); wait; ps -C main
```
Esperado: `real exit=0` e última linha completa nas corridas; no terceiro,
`child <pid> killed by signal`, `real exit=1` e `ps` vazio.
