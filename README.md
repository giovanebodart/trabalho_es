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
mingw32-make benchmark
mingw32-make plots
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

O alvo `stress` executa a suíte normal e uma carga gradual de alocações até
`SCALE_STRESS_MAX`, cujo padrão é `100000`, além de um teste de fogo
determinístico até `FIRE_STRESS_MAX`, cujo padrão é `50000`. Para observar
milhões de objetos, use:

```powershell
mingw32-make benchmark SCALE_BENCHMARK_MAX=1000000
.\build\bench_scale_allocations.exe --full
.\build\bench_fire_test.exe --full
.\build\bench_tree.exe --full
.\build\bench_scale_allocations.exe 100000 --csv > data\scale.csv
.\build\bench_fire_test.exe 50000 --csv > data\fire.csv
.\build\bench_tree.exe 100000 --csv > data\tree.csv
.\build\bench_compare_collectors.exe 50000 --csv > data\collectors.csv
```

O modo `--full` dos benchmarks também percorre `10^7` objetos, portanto deve
ser usado apenas depois de validar os estágios menores no ambiente atual.

Para regenerar os CSVs e os gráficos SVG sem editar valores manualmente:

```powershell
mingw32-make plots
```

Se `python` não estiver no `PATH`, informe o executável explicitamente com
`PYTHON=C:/caminho/para/python.exe`. O alvo cria `data/scale.csv`,
`data/fire.csv`, `data/tree.csv`, `data/collectors.csv` e os gráficos
`plots/pause_vs_heap.svg`, `plots/memory_vs_progress.svg`,
`plots/tree_cost_vs_objects.svg` e `plots/collector_pause_comparison.svg`.

O alvo de sanitização usa Clang/LLVM com AddressSanitizer e
UndefinedBehaviorSanitizer:

```powershell
mingw32-make sanitize
```

Esse alvo requer os pacotes `mingw-w64-clang-x86_64-clang` e
`mingw-w64-clang-x86_64-compiler-rt` do ambiente `clang64` do MSYS2. Por
padrão, o `Makefile` procura essa cadeia em `C:/msys64/clang64`; outro caminho
pode ser informado com `SAN_ROOT`.

Dr. Memory não está disponível no ambiente atual; quando instalado, deve ser
executado como validação complementar aos sanitizadores.

## Visualizador da árvore

O menu interativo gera todos os intervalos e endereços automaticamente:

```powershell
.\scripts\run_tree_visualizer.ps1
```

Use `-Demo` para executar uma sequência automática ou `-BuildOnly` para apenas
compilar. O script usa GCC em modo debug com as mesmas flags estritas do
projeto, além de símbolos, otimização desativada e frame pointers preservados.

## Visualizador do coletor

O visualizador ASCII mostra raizes, referencias, objetos alcancaveis, lixo,
retencoes conservadoras e metricas da pausa com
`.\scripts\run_gc_visualizer.ps1`; dados e remocoes sao aleatorios. Use `-Demo`
para uma sequencia automatica ou `-BuildOnly` para somente compilar em debug.

## Estado atual

O repositório está na Fase 8, Commit 39, comparando mark-sweep puro e coletor
geracional sobre a mesma carga sintética. O alocador usa
classes de tamanho para objetos pequenos, com blocos de 32, 64, 128, 256, 512 e
1024 bytes distribuídos a partir de arenas de 64 KiB. Objetos maiores recebem
mapeamentos dedicados por `VirtualAlloc()`.

A árvore de intervalos continua representando cada objeto individual, não a
arena inteira. As estatísticas agora expõem `bytes_internal_fragmentation`, que
mede a diferença entre bytes reservados para blocos vivos e bytes solicitados
vivos. O sweep devolve blocos pequenos às freelists, mantém arenas parcialmente
ocupadas para reuso e libera arenas inteiramente vazias quando o último objeto
vivo daquela arena é coletado.

As alocações novas entram na geração jovem. `gc_collect()` executa uma coleta
menor: marca raízes explícitas, pilha, registradores e o remembered set; em
seguida varre apenas objetos jovens não marcados. Objetos jovens promovem para
a geração antiga após
`GC_DEFAULT_PROMOTION_THRESHOLD` sobrevivências, cujo padrão é 2 e pode ser
ajustado com `gc_set_promotion_threshold()`.

O coletor mantém uma tabela interna de páginas antigas candidatas à proteção:
por enquanto entram apenas objetos antigos em mapeamentos dedicados por
`VirtualAlloc()`. Objetos antigos pequenos que vivem em arenas não são
protegíveis ainda, porque suas páginas podem conter jovens, blocos livres e
ponteiros de freelist usados internamente pelo alocador.

No build normal, páginas antigas protegíveis são marcadas como somente leitura
com `VirtualProtect()`. A primeira escrita válida instala a barreira: o tratador
vetorizado marca a página como suja e restaura permissão de escrita. Em builds
com AddressSanitizer, essa proteção fica desativada para evitar conflito entre
SEH, shadow memory e páginas protegidas pelo runtime do sanitizador.

Durante uma coleta menor, o remembered set é formado pelas páginas antigas
sujas. O coletor examina essas páginas em busca de referências para jovens,
enquanto objetos antigos pequenos ainda são escaneados conservadoramente por
não estarem protegidos por página. Após a coleta, a tabela é reconstruída,
limpa e reprotegida quando a barreira está ativa.

`gc_collect()` executa uma coleta maior completa após
`GC_DEFAULT_MAJOR_COLLECTION_INTERVAL` coletas menores. A coleta maior marca a
partir das raízes normais e varre jovens e antigos, permitindo recuperar lixo
promovido. As estatísticas diferenciam contagem e última pausa de coletas
menores e maiores.

Em builds de debug, a coleta valida canários e invariantes da árvore de
intervalos antes e depois de marcar/varrer. Corrupção detectada interrompe a
coleta com `GC_STATUS_CORRUPTED_MEMORY`.

O benchmark `bench_scale_allocations` percorre estágios graduais de escala,
observando tempo total, pausa da coleta, memória reservada no pico, bytes vivos
após a coleta e bytes recuperados em uma tabela com legenda no terminal.
O benchmark `bench_fire_test` cria ciclos e referências cruzadas
determinísticas, mantém raízes verificadas por canários, remove subconjuntos de
raízes e executa coletas menores e maiores para verificar sobrevivência dos
objetos vivos. Ambos aceitam `--csv` e emitem colunas para algoritmo, semente,
objetos, heap, bytes vivos, bytes coletados, pausa, marcação, sweep, buscas e
comparações na árvore, coletas menores e maiores, promoções, páginas sujas e
memória residente máxima amostrada pelo Windows.

O benchmark `bench_tree` mede inserção, busca e remoção na árvore de
intervalos com tamanhos crescentes, três repetições e sementes fixas. Ele
registra altura observada, `ceil(log2(n))`, tempo por operação agregada em
ticks e média de comparações nas buscas para apoiar a comparação experimental
com o comportamento `O(log n)`.

O benchmark `bench_compare_collectors` executa aquecimento e três repetições da
mesma carga em mark-sweep puro e no coletor geracional, emitindo média, mediana
e dispersão das pausas e do tempo total em tabela ou CSV.

O script `scripts/generate_plots.py` consome os CSVs gerados pelos benchmarks e
produz gráficos SVG reproduzíveis usando apenas a biblioteca padrão do Python.

## Documentação do desenvolvimento

O roteiro completo está em `PLAN.md`. Decisões, testes, limitações e prompts
relevantes são registrados cronologicamente em `DIARIO.md`.
