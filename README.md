# Coletor de lixo geracional em C11

Projeto acadêmico de um coletor de lixo conservador para Linux x86-64. A
implementação será desenvolvida incrementalmente, começando por mark-sweep e
evoluindo para duas gerações com barreira de escrita baseada em `mprotect()`.

## Objetivos principais

- disponibilizar uma API de alocação baseada em `gc_malloc()`;
- obter o heap gerenciado por meio de `mmap()`;
- localizar ponteiros interiores em `O(log n)` com uma árvore AVL de
  intervalos;
- identificar raízes explícitas, da pilha e dos registradores;
- preservar ciclos alcançáveis e recuperar ciclos mortos;
- comparar experimentalmente mark-sweep puro e coleta geracional;
- executar cargas graduais de até `10^7` objetos.

## Plataforma e restrições

- Linux x86-64;
- processo com uma única thread;
- C11;
- GCC com `-std=c11 -Wall -Wextra -Werror -pedantic`;
- zero warnings;
- APIs POSIX/Linux documentadas quando necessárias.

## Estrutura

- `include/`: cabeçalhos públicos e internos;
- `src/`: implementação do coletor;
- `tests/`: testes automatizados;
- `benchmarks/`: medições e teste de fogo;
- `examples/`: programas-cobaia;
- `scripts/`: automação de benchmarks e gráficos;
- `data/`: dados experimentais;
- `plots/`: gráficos gerados.

## Estado atual

O repositório está na Fase 0, Commit 1, definida em `PLAN.md`. Neste momento há
somente a fundação documental e a estrutura de diretórios; ainda não existe
implementação do coletor nem sistema de build.

O próximo passo é o Commit 2: criar o `Makefile`, habilitar as flags estritas e
adicionar um executável de teste mínimo com os alvos `all`, `test`, `stress` e
`clean`.

## Documentação do desenvolvimento

O roteiro completo está em `PLAN.md`. Decisões, testes, limitações e prompts
relevantes são registrados cronologicamente em `DIARIO.md`.
