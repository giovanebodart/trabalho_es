# Coletor de lixo geracional em C11

Projeto acadêmico de um coletor de lixo conservador para Windows 11 x86-64. A
implementação será desenvolvida incrementalmente, começando por mark-sweep e
evoluindo para duas gerações com barreira de escrita baseada em proteção de
páginas da memória virtual.

## Objetivos principais

- disponibilizar uma API de alocação baseada em `gc_malloc()`;
- obter o heap gerenciado por meio das APIs de memória virtual do Windows;
- localizar ponteiros interiores em `O(log n)` com uma árvore AVL de
  intervalos;
- identificar raízes explícitas, da pilha e dos registradores;
- preservar ciclos alcançáveis e recuperar ciclos mortos;
- comparar experimentalmente mark-sweep puro e coleta geracional;
- executar cargas graduais de até `10^7` objetos.

## Plataforma e restrições

- Windows 11 x86-64;
- processo com uma única thread;
- C11;
- GCC do MSYS2/MinGW-w64 com
  `-std=c11 -Wall -Wextra -Werror -pedantic`;
- zero warnings;
- dependências da API Win32 documentadas quando necessárias.

## Estrutura

- `include/`: cabeçalhos públicos e internos;
- `src/`: implementação do coletor;
- `tests/`: testes automatizados;
- `benchmarks/`: medições e teste de fogo;
- `examples/`: programas-cobaia;
- `scripts/`: automação de benchmarks e gráficos;
- `data/`: dados experimentais;
- `plots/`: gráficos gerados.

## Build inicial

Em Windows 11 x86-64, com GCC e GNU Make do MSYS2/MinGW-w64:

```powershell
mingw32-make clean
mingw32-make all
mingw32-make test
mingw32-make stress
```

O build utiliza as flags obrigatórias
`-std=c11 -Wall -Wextra -Werror -pedantic`. Os artefatos são gravados em
`build/` e removidos por `mingw32-make clean`.

O Commit 2 foi validado no ambiente Windows. A validação em Linux não foi
realizada porque não há distribuição WSL instalada e deixou de ser obrigatória
após a mudança da plataforma de desenvolvimento.

Os testes ficam separados por módulo em `tests/test_*.c` e usam as macros de
`tests/test.h`. Uma asserção malsucedida retorna `EXIT_FAILURE`, permitindo que
o GNU Make interrompa a suíte com código diferente de zero.

O alvo de sanitização usa Clang/LLVM com AddressSanitizer e
UndefinedBehaviorSanitizer:

```powershell
mingw32-make sanitize
```

Esse alvo requer os pacotes `mingw-w64-clang-x86_64-clang` e
`mingw-w64-clang-x86_64-compiler-rt` do ambiente `clang64` do MSYS2. Por
padrão, o `Makefile` procura essa cadeia em `C:/msys64/clang64`; outro caminho
pode ser informado com `SAN_ROOT`.

## Estado atual

O repositório está na Fase 0, Commit 3, definida em `PLAN.md`. Há um sistema de
build, testes separados por módulo e configuração de sanitizadores, mas ainda
não existe implementação do coletor.

O próximo passo é o Commit 4: definir o nó da árvore de intervalos e suas
operações auxiliares básicas.

## Documentação do desenvolvimento

O roteiro completo está em `PLAN.md`. Decisões, testes, limitações e prompts
relevantes são registrados cronologicamente em `DIARIO.md`.
