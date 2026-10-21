# Lab 4 — notas de sessão (tutor)

Última sessão: 2026-10-21. Objetivo: **corrigir os erros do lab4**.

- Enunciado: `docs/lab4.pdf` (depende do modelo do `docs/lab2.pdf` e da temporização do `docs/lab3.pdf`).
- Código: `lab/main.c` (a pasta `lab2/` foi renomeada para `lab/`), biblioteca em `lib/`, header em `include/calculus.h`.
- Build: `make` em `lib/` **antes** (o `librobo.a` foi apagado no commit "Lab 4"), depois `make` em `lab/`.
- Execução: `./main <arquivo_saida>.dat` (20 s).

## Estrutura do programa

3 processos via `fork()`, trocando só `u` e `y` por pipes não-bloqueantes:

| Processo | Período | Papel |
|---|---|---|
| gen_u (filho 1) | 50 ms | gera u(t), escreve no pipe A |
| simulator (pai) | 30 ms | lê u do pipe A, calcula x e y, escreve y no pipe B |
| sampling (filho 2, filho do simulator) | 50 ms | lê y do pipe B (y(k)) e grava no arquivo |

## Plano de correção

| # | Passo | Status |
|---|---|---|
| 1 | Tipos no pipe/biblioteca + inicialização de `u` e `y_value` | ✔ tudo em `double`; `u = {0,0}`, `y_value = {0.3,0,0}` |
| 2 | `DT` coerente com o período de 30 ms | ✔ `DT 0.03` |
| 3 | Tempo como inteiro (`int64_t`, µs, `PRId64`) | ✔ zero warnings, tempo exato no arquivo |
| 4 | Fechar pontas de pipe não usadas + `wait` dos filhos | **← PRÓXIMO** |
| 5 | Registrar u(t) e y(t) (hoje só y(k) é salvo); T(k)/J(k) de **cada** tarefa | pendente |
| 6 | Temporização absoluta (t0 + k·T) para eliminar drift | pendente |
| 7 | Tudo em inglês (comentários, `printf`, variáveis, Python) | pendente |

## Pergunta em aberto (passo 4) — começar por aqui

Preencher a tabela com **usa / tem mas não usa (fechar) / não tem**:

| | pipe A `[0]` | pipe A `[1]` | pipe B `[0]` | pipe B `[1]` |
|---|---|---|---|---|
| gen_u | ? | ? | ? | ? |
| simulator | ? | ? | ? | ? |
| sampling | ? | ? | ? | ? |

Dicas: o pipe A é criado **antes** do 1º `fork`, o pipe B **depois**; o filho herda todos os descritores abertos do pai.
Depois: quem é pai de quem e quem deve esperar (`wait`) quem?

## Pendências menores

- Comentários em português: `lab/main.c` linhas ~99, 102, 105, 126, 131; mensagens de `printf`; `seno_matriz`/`cos_matriz` em `lib/`; `include/calculus.h:6`; `lab/gerar_graficos.py` inteiro.
- Números mágicos `30000` e `50000` → constantes com unidade (`_US`). `DT` e período de 30 ms deveriam derivar um do outro.
- `0.3` em `y_value` depende de `D` (em `lib/result_y.c`) — opcional: calcular com `result_y` a partir de x = 0.
- `lib/derivate.c` ignora `v = u[0]` (funciona só porque v = 1).
- Conversão `timeval → µs` repetida 7 vezes → candidata a função (desafio de design em aberto).
- `lab/get_u_t.c` e `lab/write_file.c` estão sem uso e com `float`; decidir o destino.
- Perguntas de verificação não respondidas: (1) guardar em ns ganha informação se o relógio dá µs? (2) o que é o problema Y2038?

## Medição de referência (antes do passo 6)

Execução sem carga, após o passo 3 (amostragem, 400 amostras):
T(k) min 50 059 µs, max 50 307 µs; último t = 20 011 892 µs vs ideal 19 950 000 µs → **drift ≈ 62 ms**.
Nenhum período ficou abaixo de 50 ms (o erro do `usleep` relativo só acumula).

## Notas para o tutor

- Explicar o **mecanismo** com números do próprio código antes de dar roteiro; o aluno pediu explicitamente para ser ensinado, não só guiado.
- Exemplos genéricos curtos (2–3 linhas) ajudaram (ex.: sintaxe de `PRId64`).
- O aluno às vezes esquece de salvar o arquivo no editor antes de pedir revisão.
- Professor sugeriu `gettimeofday` e "sleep(T - (t - t0))" — manter `gettimeofday`; `sleep()` literal tem resolução de 1 s, usar `usleep`.
