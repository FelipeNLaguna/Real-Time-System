# Lab 7 — notas de sessão (tutor)

Última atualização: 2026-11-17 (dia da entrega). Ler **antes** de qualquer coisa
ao retomar. O jeito de trabalhar (senior/junior, estilo de review, regras)
continua o de `docs/lab5-session-notes.md`, seção "Como trabalhamos".

## Enunciado (`docs/lab7.pdf`) — entrega **17/11/2026 até 23h59** (Moodle) + apresentação

Trocar a comunicação entre tarefas por **memória compartilhada**, protegendo
`y`, `u` e demais variáveis com **semáforos POSIX**. Fontes soltos (não zipados)
+ relatório. Lab 6 não foi passado: base = lab 5 (`bee9764`, processos + pipes).
Aluno trabalha até ~15h30 no código; depois medições e relatório.

## Decisões do aluno

- **Threads** (`pthread`), `sem_init(..., pshared = 0)`. Nada de `shm_open`.
- Cada tarefa num `.c` em `lib_rt_tasks/` (→ `librealtime.a`), buffer em
  `lib_buffer/` (→ `libbuf.a`), cálculo em `lib_calculus/` (→ `librobo.a`),
  headers em `include/`, `lab/main.c` só cria/junta threads.
- **Git:** branch `lab7-threads` (`main` = lab 5 entregue). Commits mínimos.
- **Liberação síncrona** (`t0 = agora + margem`, threads dormem até `t0`)
  fica para o **final de tudo**. gen_u já trata `t < t0` com `u = 0`.
- Robustez extra (🟠) fica como limitação conhecida no relatório.

## Desenho (`struct data_tasks` em `include/rt_tasks.h`, uma instância no `main`)

| Objeto | Escreve | Lê | Proteção / valor inicial |
|---|---|---|---|
| `timer_init` (t0), `file_name` (`argv[1]`) | main antes das threads | todas / logger | só leitura |
| `u_rt[2]` (init 1, 0.2π) | gen_u | simulator | `sem_variable_u` = 1, `sem_wait` |
| `y_rt[3]` (init 0.3, 0, 0 = `result_y(x0)`) | simulator | sampling | `sem_variable_y` = 1, `sem_wait` |
| fila `Buffer buf` (`BUFFER_SIZE 1170` ≈ 73 reg/s × 16 s) | 3 RT | logger | `sem_buffer` = 1, `can_send` = `BUFFER_SIZE - 1`, `can_save` = 0 |
| display | gen_u (u), sampling (y) | display | **a fazer** (recomendado abaixo) |

- RT na fila: `sem_trywait(can_send)` → `sem_trywait(sem_buffer)`; `EAGAIN` =
  `dropped++` (e devolve `can_send` se falhou o 2º); `buffer_append` < 0 =
  bug → devolve `sem_buffer` e sai com erro. `fprintf` dos descartes após o laço.
- Justificativa (relatório): `sem_wait` só onde todos os disputantes são RT e
  a seção é curta (u: gen_u 80 × sim 90); `trywait` onde um não-RT disputa
  (fila: logger) → sem espera = sem inversão; custo = descarte. Semáforo POSIX
  não tem herança de prioridade (professor citou inversão). gen_u e sampling
  têm mesmo período e fase → podem colidir no `sem_buffer` e descartar sem
  culpa do logger: olhar contadores.
- Display recomendado: área própria "último u / último y" + semáforo próprio;
  RT publicam com `sem_trywait` (pula se ocupado); display `SCHED_OTHER`
  periódico (100–500 ms) copia sob o semáforo e imprime fora. Relatório: deixou
  de ser por evento.
- Validação: trajetória (`y[1]` × `y[0]`) deve ser um **8** (dois círculos de
  raio 1/(0,2π) ≈ 1,59, 10 s cada, sentidos opostos); 400/667/400 linhas.

## Estado em 17/11 (antes do commit "main + Makefiles")

- ✅ simulator, gen_u, sampling: corretos (`a15ede2`, `7d0f89c`). Limpeza
  pendente: comentário "I think DT..." no simulator, `#include "calculus.h"`
  sobrando no sampling, msg "%d dropped by logger" sem "records", `>= 0`
  redundante e indentação no gen_u, parâmetro `data_tasks` com nome do tipo.
- `lab/main.c`: `create_threads` com `EXPLICIT_SCHED` + política + prioridade +
  `strerror` ✅; `&data_main` como arg ✅; `PRIO_*` ✅; `file_name` ✅.
  Falta: `%s` com `int` (340–356, 5 warnings); `timespec_*` duplicadas (57–68,
  não dá erro de link mas a cópia do main ganha); `run_display`/`run_logger`
  antigos (70–262) → mover lógica de arquivos para `logger.c` e apagar;
  includes do lab 5; retornos de `sem_init`/`pthread_join` não checados (🟠).
- Makefiles: `lab/Makefile:7` `-librealtime`/`-libbuf` errados (o `-l` já
  põe `lib`); ordem tem que ser quem usa antes de quem define
  (realtime → buf, robo → m); `-pthread` falta no link; `lib_buffer/Makefile`
  tem `-pthread` no `INCLUDE`; `main` não depende das `.a` → usar `make -B`.
- `logger.c` e `display.c` **vazios** → próximo erro esperado:
  `undefined reference to logger/display`.
- **Término do logger (pendente):** ideia dele (`time_us > 20 s` e buffer
  vazio) não funciona — último `time_us` < 20 s e logger fica preso em
  `sem_wait(can_save)`. Direção dada: o `main` sabe que as RT acabaram depois
  dos `pthread_join` → aviso na struct + `sem_post(can_save)` para acordar;
  logger esvazia a fila antes de sair.
- `result_y.c`: nomes já em inglês; bloco comentado `matrix_y` a apagar.

Build (da raiz):
```
make -B -C lib_calculus && make -B -C lib_buffer && make -B -C lib_rt_tasks && make -B -C lab
```

## Próximos passos

Makefile (`-l` e ordem) → logger (consumidor da fila + término) → rodar
(teste rápido possível sem logger/display: fila enche em ~16 s, conta de
descartes) → display → liberação síncrona → medições (procedimento do RT-13
nas notas do lab 5) → relatório.
