# Lab 7 — notas de sessão (tutor)

Última atualização: 2026-11-16. Ler **antes** de qualquer coisa ao retomar.
O jeito de trabalhar (senior/junior, estilo de review, regras) continua o de
`docs/lab5-session-notes.md`, seção "Como trabalhamos".

## Enunciado (`docs/lab7.pdf`) — entrega **17/11/2026** (Moodle + apresentação)

Trocar a comunicação entre tarefas do lab anterior por **memória
compartilhada**, protegendo `y`, `u` e demais variáveis com **semáforos POSIX**.
Fontes soltos (não zipados) + relatório. O lab 6 não foi passado: a base é o
código do lab 5 (`bee9764`, processos + pipes).

## Decisões do aluno (16/11)

- **Threads** (`pthread`), memória compartilhada = espaço do processo,
  `sem_init(..., pshared = 0)`. Nada de `shm_open`.
- Cada tarefa num `.c` próprio (`lib_rt_tasks/`), headers em `include/`, `main`
  só cria/junta threads. `lib/` virou `lib_calculus/` (`librobo.a`).
- Pulou o RT-14 "threads mantendo pipes": foi direto para memória compartilhada,
  começando pelo simulator.
- **Git:** branch `lab7-threads` (código ainda não compila; `main` = lab 5
  entregue). Commits mínimos por causa do prazo.
- **Liberação síncrona** (todas as threads dormem até `t0 = agora + margem`
  definido pelo `main` antes do `pthread_create`) fica para o **final de tudo**.

## Desenho acordado

`struct data_tasks` (em `include/rt_tasks.h`), **uma instância** criada no
`main`, mesmo ponteiro para as 5 threads:

| Objeto | Escreve | Lê | Proteção |
|---|---|---|---|
| `t0` (`timer_init`) | main (antes das threads) | todas | só leitura |
| `u_rt[2]` | gen_u | simulator | `sem_variable_u` (binário) |
| `y_rt[3]` | simulator | sampling | `sem_variable_y` (binário) |
| fila de log `Buffer buf` | 3 RT | logger | `sem_buffer` (exclusão), `can_send` (vagas, init `BUFFER_SIZE - 1`), `can_save` (itens, init 0) |
| display | gen_u (u), sampling (y) — como no lab 5 | display | **a decidir** |

- Buffer circular em `lib_buffer/buffer.c` (`in`/`out`, módulo, uma vaga
  sempre vazia). `BUFFER_SIZE 1170` ≈ 73 registros/s × 16 s (igual ao pipe).
- Produtor RT: `sem_trywait(can_send)` → `EAGAIN` = `dropped++`; depois
  `sem_trywait(sem_buffer)` → se falhar, **devolver** `can_send`. Logger pode
  usar `sem_wait` (não-RT). `fprintf` dos descartes **depois** do laço.
- Display (pergunta aberta para ele): ler direto `y_rt`/`u_rt` põe uma tarefa
  `SCHED_OTHER` disputando semáforo com o simulator (90) que usa `sem_wait` →
  inversão sem limite; e display era por evento (como saber que há valor novo?).
- Para o relatório: semáforo POSIX **não tem herança de prioridade** (mutex
  tem); seções críticas mínimas ("monta local, publica com uma cópia").

## Estado do `lib_rt_tasks/simulator.c` (compila limpo com `gcc -fsyntax-only -Wall -Wextra -Iinclude`)

Pronto: cast do `void *`, seção de `y` (wait com retry `EINTR` + checagem,
publica `y_rt[0..2]`), fila de log com devolução de `can_send`, `errno` lido
antes do `sem_post`, `fprintf` dos descartes após o laço, retorno
`(void *)(intptr_t)`.

Falta:
1. Seção de `u` (linhas ~45–54): checar `ret_wait_u` após o laço de `EINTR`;
   mensagem da linha 51 diz `sem_wait` mas é `sem_post`.
2. **Item 6 (ele travou aqui):** retorno do `buffer_append` (linha ~99) —
   condição `< 0` nunca é verdadeira (retorna 0/1); falha = contagem
   semáforo×buffer quebrada = bug → fatal, **sem** `sem_post(can_save)`, sem
   `perror` (não mexe em `errno`), e **devolver `sem_buffer` antes de sair**
   (senão logger/`pthread_join` travam). Dei níveis 1 e 2; próximo é o 3.
   Mesma pergunta vale para os outros `return` dentro de seção crítica.
3. Erro do `clock_nanosleep` só imprime e segue (não dorme → laço FIFO 90 sem
   pausa).
4. Limpeza: comentário "I think DT..." (~86), atribuição de struct inteira
   (13–14), parâmetro `data_tasks` com nome do tipo (sugestão `arg`), `res_sim`.
5. Sincronização (final de tudo).

Outros arquivos:
- `rt_tasks.h`: `sem_logger` e `log_thread` sobrando; comentários "receive 3
  structs" desatualizados; nomes `sem_buffer`/`can_send`/`can_save` pouco claros.
- `buffer.h`: comentário `// int = 4 bytes` errado; documentar conta do 1170 e
  relação com `BUFFER_SIZE - 1`. `buffer.c:7`: comentário diz "free slots" mas
  retorna ocupados.
- `lab/main.c`: ainda é o do lab 5, só ganhou `#include "rt_tasks.h"` →
  conflito de `struct log_record` (não compila). `timespec_utils.c` criado.
- gen_u, sampling, logger, display: ainda vazios/não revisados.

## Próximos passos

simulator (itens acima) → gen_u e sampling (mesmo padrão) → logger (fila,
término: como acordar o logger quando as RT acabam?) → display → `main`
(`sem_init` antes das threads, atributos `PTHREAD_EXPLICIT_SCHED` + FIFO
90/80/70, logger/display `SCHED_OTHER`, `mlockall` **antes** de criar threads,
`pthread_join` checando retorno) → Makefile com todos os `.c` + `-pthread` →
sincronização → testes (400/667/400, `ps -Lo pid,lwp,policy,rtprio,comm`) →
relatório.

Em casa: `librobo.a` não está versionado → `make -C lib_calculus` antes.
