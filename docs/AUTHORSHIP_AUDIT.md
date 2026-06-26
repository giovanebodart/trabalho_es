# Auditoria de autoria e defesa

Este documento organiza o Commit 43 do `PLAN.md`: preparar a equipe para
explicar o projeto, revisar trechos criticos e registrar evidencias de autoria.
Ele nao deve ser preenchido pela IA em nome dos integrantes. Cada pessoa deve
usar este roteiro para estudar, explicar e fazer uma pequena alteracao propria
sem auxilio externo antes da defesa.

## Objetivo

Confirmar que a equipe consegue:

- explicar a arvore AVL de intervalos;
- explicar marcacao, sweep, pilha, registradores e raizes explicitas;
- explicar geracoes, remembered set, `VirtualAlloc()` e `VirtualProtect()`;
- interpretar metricas, benchmarks e graficos;
- identificar limitacoes e riscos sem depender da IA;
- realizar ao menos uma modificacao pequena, rastreavel e defensavel por
  integrante.

## Mapa minimo de defesa

| Tema | Arquivos principais | O que a pessoa deve saber explicar |
|---|---|---|
| Arvore AVL de intervalos | `include/interval_tree.h`, `src/interval_tree.c`, `tests/test_interval_tree.c` | Intervalo semiaberto `[start, end)`, rejeicao de sobreposicao, rotacoes, altura, `max_end` e busca por ponteiro interior. |
| Metadados e heap | `src/allocator.c`, `src/allocator.h`, `include/gc_stats.h` | Separacao entre metadados e area do usuario, arenas, classes de tamanho, mapeamentos dedicados, canarios e fragmentacao interna. |
| API e estado global | `include/gc.h`, `src/gc.c` | Estados de erro, thread dona, ciclo de vida, limite de memoria e politica de coleta maior. |
| Marcacao conservadora | `src/marker.c`, `src/marker.h` | Fila iterativa, leitura com `memcpy()`, ponteiros interiores, falsos positivos e ausencia de recursao profunda. |
| Raizes explicitas | `src/roots.c`, `src/roots.h`, `tests/test_gc.c` | Por que guardar o endereco da variavel-raiz e nao apenas o valor atual. |
| Pilha | `src/stack_roots.c`, `tests/test_stack_roots.c` | Limites suportados, dependencia de ABI e risco de otimizacao do compilador. |
| Registradores | `src/register_roots.c`, `tests/test_register_roots.c` | Uso de `setjmp()`, tratamento conservador do `jmp_buf` e limitacoes. |
| Sweep | `src/sweeper.c`, `src/sweeper.h`, `tests/test_sweeper.c` | Preservacao de marcados, coleta de nao marcados, promocao e devolucao de blocos. |
| Geracoes | `src/gc.c`, `src/sweeper.c`, `include/gc_config.h` | Jovem/antiga, limiar de promocao, coleta menor e coleta maior periodica. |
| Barreira de escrita | `src/old_pages.c`, `src/old_pages.h`, `src/gc.c` | `VirtualProtect()`, pagina suja, remembered set, tratador minimo e conflito com sanitizadores. |
| Medicoes | `include/gc_stats.h`, `benchmarks/*.c`, `scripts/generate_plots.py` | Pausas em ticks, mark/sweep, buscas na arvore, RSS, CSVs e graficos SVG. |

## Perguntas que cada integrante deve responder

1. Por que a arvore representa objetos individuais e nao apenas arenas?
2. Qual e a convencao de intervalo usada pela arvore?
3. Como `max_end` reduz o custo da busca conservadora?
4. Por que a marcacao usa fila e nao recursao?
5. Como o coletor evita coletar ciclos ainda alcancaveis?
6. O que e um falso positivo conservador e por que ele e aceitavel?
7. Por que raizes explicitas guardam `void **`?
8. O que `setjmp()` fornece para a varredura de registradores?
9. Quando uma coleta menor pode preservar um jovem referenciado por antigo?
10. Qual e a diferenca entre coleta menor e coleta maior neste projeto?
11. Por que nem todo objeto antigo pequeno e protegido com `VirtualProtect()`?
12. Quais operacoes nao devem acontecer dentro do tratador de excecao?
13. Por que a barreira fica desativada em build com AddressSanitizer?
14. Como o teste de fogo demonstra recuperacao de ciclos mortos?
15. Por que o benchmark geracional pode ser pior que mark-sweep em uma carga?

## Roteiro de revisao coletiva

1. Executar `git status --short --branch` e confirmar arvore limpa.
2. Executar `mingw32-make clean all test sanitize stress benchmark`.
3. Executar `mingw32-make plots` ou informar `PYTHON=...` se necessario.
4. Abrir `docs/TECHNICAL.md` e cada integrante apresentar um modulo.
5. Abrir `docs/REPORT.md` e cada integrante interpretar um resultado.
6. Revisar os pontos marcados como limitacao: pilha, registradores, falsos
   positivos, objetos antigos pequenos em arenas e Dr. Memory ausente.
7. Registrar duvidas que ainda nao estejam compreendidas pela equipe.
8. Cada integrante faz uma modificacao pequena sem IA, executa os testes
   pertinentes e registra a evidencia abaixo.

## Protocolo para modificacao individual sem IA

Cada modificacao deve ser pequena, explicavel e de baixo risco. Exemplos
aceitaveis:

- adicionar um caso simples de teste em modulo ja existente;
- melhorar uma mensagem de benchmark sem alterar metrica;
- acrescentar uma nota tecnica em `docs/TECHNICAL.md`;
- ajustar comentario que a pessoa consiga explicar;
- adicionar uma verificacao documental no relatorio.

Evitar:

- refatoracao ampla;
- mudanca em politica de coleta sem teste;
- alterar CSVs ou graficos manualmente;
- remover validacoes para fazer passar;
- copiar trecho gerado por IA.

Modelo de registro por integrante:

| Integrante | Tema explicado | Arquivo alterado sem IA | Teste executado | Resultado | Revisado por |
|---|---|---|---|---|---|
| A preencher | A preencher | A preencher | A preencher | A preencher | A preencher |
| A preencher | A preencher | A preencher | A preencher | A preencher | A preencher |
| A preencher | A preencher | A preencher | A preencher | A preencher | A preencher |

## Trechos que merecem revisao cuidadosa

- `interval_node_rotate_left()` e `interval_node_rotate_right()`: confirmar que
  atualizam altura e `max_end` na ordem correta.
- `interval_tree_remove()`: confirmar que remocao com dois filhos preserva a
  ordenacao e reequilibra a arvore.
- `gc_mark_scan_object()`: confirmar uso de `memcpy()` e busca por ponteiro
  interior.
- `gc_collect()`: confirmar a ordem de preparacao, marcacao, remembered set,
  sweep, estatisticas e reprotecao.
- `gc_sweep_young()` e `gc_sweep_all()`: confirmar diferenca entre coleta menor
  e maior.
- `gc_old_pages_handle_write()`: confirmar que o tratador marca a pagina suja e
  restaura escrita sem operacoes complexas.
- `gc_register_roots_capture()`: confirmar que o projeto nao depende do layout
  interno de `jmp_buf`.
- `scripts/generate_plots.py`: confirmar que os graficos usam CSVs gerados e
  nao valores editados manualmente.

## Criterio de pronto para a defesa

A auditoria deve ser considerada pronta quando:

- todos os integrantes preencherem sua linha de evidencia;
- cada tema do mapa minimo tiver ao menos uma pessoa responsavel;
- a suite obrigatoria passar em ambiente limpo;
- nenhuma duvida critica permanecer sem dono;
- a equipe conseguir explicar uma limitacao sem prometer suporte nao validado;
- o historico Git e o `DIARIO.md` mostrarem o caminho incremental do projeto.
