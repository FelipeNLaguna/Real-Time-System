# Projeto final — notas de sessão (tutor)

Última atualização: 2026-11-23. Ler **antes** de qualquer coisa ao retomar.
O jeito de trabalhar (senior/junior por tickets, estilo de review, regras)
continua o de `docs/lab5-session-notes.md`, seção "Como trabalhamos".
Estado final do lab 7 (base do projeto): `docs/lab7-session-notes.md`.

## Enunciado (`docs/prj.pdf`) — entrega **10/12/2026** (Moodle) + apresentação

Robô diferencial com controle em cascata. Tarefas e períodos:

| Tarefa | Período | Entra | Sai | Tipo |
|---|---|---|---|---|
| (a) dinâmica (u̇ = f(u) + Fτ) | 1 ms | τ | u | planta |
| (c) linearização τ = F⁻¹(v − f(u)) | 1 ms | v, u | τ | controle |
| (d) PI de v (Kp 184, Ki 8464) | 2 ms | ur1, u1 | v1 | controle |
| (e) PI de ω | 2 ms | ur2, u2 | v2 | controle |
| (b) cinemática (ẋ = B(x3) u) | 10 ms | u | x, y | planta |
| (f) transformação (8) | 10 ms | x, xr | x̄ | controle |
| (g) polar (9–11) | 10 ms | x̄ | e, ψ, α | controle |
| (h) pose (12–13), γ1 10, γ2 1, λ2 0,1, λ3 0,1 | 10 ms | e, ψ, α | ur1, ur2 | controle |
| (i) referência (xr, yr, θr; troca em 10 s) | 50 ms | t | xr | controle |
| (j) interface + disco | — | tudo | tela, arquivos | não-RT |

x(0) = [0 1 0]ᵀ. Entregáveis: gráfico de y(k) com xr, yr (20 s); tabela de
**tempo de computação** e seu jitter (média, var, std, mín, máx) sem/com
`hackbench`, por tarefa; `ps -axHo pid,policy,ni,pri,rtprio,pcpu,psr,cmd`;
**análise de escalonabilidade por tempo de resposta** (RTA, usa WCET medido);
fontes soltos + relatório PDF.

## Base

Branch `lab7-threads` (`16d4d4a`): threads, memória compartilhada +
semáforos, logger com fila + display periódico não-RT, `clock_nanosleep`
absoluto, `mlockall`, `create_threads` com `EXPLICIT_SCHED`. Pendências
herdadas do lab 7: liberação síncrona (t0 no futuro) não feita; medição com
`hackbench` não feita; colisão entre RT no `sem_buffer` gera descartes.

## Pontos de desenho a levantar (como perguntas, não dar resposta)

- Escala 1 ms: inversão de prioridade de 100 µs = 10 % do período →
  `sem_wait` × `trywait` × mutex com `PTHREAD_PRIO_INHERIT`.
- Volume de log: tarefas de 1 ms × 20 s = 20 000 ativações cada (~68 000
  registros de timing no total) → fila atual (1170) não comporta; alternativa:
  timing em arrays locais pré-alocados (já travados pelo `mlockall`) e gravação
  depois do laço.
- Tempo de computação = fim − início da ativação (duas leituras de relógio:
  certo para **medir**, errado para **controlar** o período).
- Prioridades por Rate Monotonic: 1 ms > 2 ms > 10 ms > 50 ms; desempate entre
  tarefas de mesmo período pelo fluxo de dados.
- Integração: dinâmica com passo = 1 ms, cinemática com passo = 10 ms
  (`DT` tem que bater com o período de cada uma).

## Decisões do aluno (23/11)

- **Só `sem_wait`** (recomendação do professor). Monitor (dado + funções que
  fazem wait/post) = melhoria se sobrar tempo. Lembrar: `sem_wait` entre
  prioridades diferentes → termo de **bloqueio B_i** na RTA (item 7).
- Ritmo **acelerado**; disponibilidade ainda indefinida.
- Git **mínimo** também no projeto (aprende git num projeto futuro): eu dou os
  comandos exatos, commit direto na branch.
- Branch **`project`** a partir de `lab7-threads`.
- Professor: **`trywait` proibido** (perdia dados). Mutex não foi proibido →
  perguntar a ele se aceita `pthread_mutex` com `PTHREAD_PRIO_PROTECT`/`INHERIT`;
  se não, emular ICPP (subir prioridade ao teto em volta do `sem_wait` com
  `pthread_setschedprio`). Decidir no PRJ-05, quando o mapa tarefa × recurso
  estiver pronto (define os tetos). RTA com teto: B_i = maior seção crítica
  de tarefa mais baixa em recurso com teto ≥ P_i.
- **Protocolo escolhido: ceiling (teto)**, preferido à herança (23/11). Ainda
  falta confirmar com o professor se mutex `PTHREAD_PRIO_PROTECT` é aceito;
  senão, emular ICPP com semáforo. Entra no PRJ-05.
- Cores: aluno quer "rodar e ver" com 1, 2, N cores → `taskset -c ...` sem
  mudar código. Execução com **1 core** é a que valida a RTA (uniprocessador);
  multi-core vira discussão. Prever: com 1 core, logger/display não-RT podem
  ficar sem CPU → fila enche.
- Fila do logger: professor mencionou "cálculo simples" → taxa de chegada ×
  tempo máximo sem o logger esvaziar. Deixar ele calcular (Σ 1/T_i).

## Roteiro (proposto)

| Ticket | O quê | Validação |
|---|---|---|
| **PRJ-01** | malha interna: (a) dinâmica, (c) linearização, (d)(e) PI, ur constante | u → ur sem sobressinal (polos duplos em −92 rad/s, assenta ~60 ms) |
| PRJ-02 | (b) cinemática | robô anda com u da malha interna |
| PRJ-03 | (i) referência + (f)(g)(h) → malha fechada | y segue xr (círculos de raio 5/π ≈ 1,59 m) |
| PRJ-04 | log/display: y(k), xr, yr + tempo de computação (arrays locais?) | gráfico item 4 |
| PRJ-05 | liberação síncrona, RM, `mlockall`, medições sem/com `hackbench`, `ps` | itens 5 e 6 |
| PRJ-06 | RTA (com termo de bloqueio) | item 7 |
| PRJ-07 | relatório | — |

## Estado

- 23/11: enunciado explicado; **PRJ-01 emitido**. Perguntas pendentes dele:
  (1) f(u) e F lidos da eq. (2) — conferir antes de codar (leitura do PDF:
  provavelmente f(u) = [0,00431·u2²; 0,18510·u1·u2]? **não afirmar**, deixar
  ele montar e conferir com o paper/enunciado); (2) polos da malha PI
  (s² + 184 s + 8464 = (s + 92)² → crítico, sem sobressinal, ~5,8/92 ≈ 63 ms);
  (3) volume de log a 1 ms.
- `docs/prj.pdf`, estas notas e o `CLAUDE.md` serão commitados por ele no
  primeiro passo do PRJ-01.
