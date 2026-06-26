# Documentacao tecnica do coletor

Este documento consolida a arquitetura atual do projeto apos a geracao dos
benchmarks e graficos. O foco e deixar claro o que existe, como validar e quais
limitacoes devem ser lembradas na defesa.

## Plataforma suportada

O projeto e validado para:

- Windows 11 x86-64;
- processo de uma unica thread;
- C11;
- GCC MinGW-w64/MSYS2 com `-std=c11 -Wall -Wextra -Werror -pedantic`;
- APIs Win32 para memoria virtual, protecao de paginas, excecoes vetorizadas e
  medicao de tempo.

Nao ha promessa de portabilidade para Linux. Uma versao Linux exigiria outra
implementacao para memoria virtual, varredura de pilha/registradores e barreira
de escrita.

## Visao geral da arquitetura

O coletor fornece uma API publica em `include/gc.h`:

- `gc_init()` inicializa o estado global, pagina do sistema, limite conhecido da
  pilha, tabela de paginas antigas e tratador da barreira;
- `gc_malloc()` aloca objetos gerenciados e registra cada intervalo na arvore;
- `gc_add_root()` e `gc_remove_root()` registram enderecos de variaveis-raiz;
- `gc_collect()` executa coleta menor ou maior conforme a politica atual;
- `gc_get_stats()` exporta metricas de memoria, pausa, arvore e geracoes;
- `gc_shutdown()` libera recursos internos e reseta a configuracao padrao.

O estado e propositalmente monothread. Chamadas vindas de outra thread sao
rejeitadas com `GC_STATUS_WRONG_THREAD`, porque a sincronizacao ainda nao faz
parte do escopo.

## Modulos principais

- `src/gc.c`: coordena o ciclo de vida, politica de coleta, raizes,
  estatisticas, barreira e transicao entre coleta menor e maior.
- `src/allocator.c`: reserva memoria com `VirtualAlloc()`, organiza arenas de
  objetos pequenos, mapeamentos dedicados para objetos grandes, metadados e
  canarios de debug.
- `src/interval_tree.c`: mantem uma arvore AVL de intervalos semiabertos
  `[start, end)` para localizar ponteiros para o inicio ou interior de objetos.
- `src/marker.c`: implementa a fila iterativa de marcacao e a varredura
  conservadora dos objetos sem recursao profunda.
- `src/roots.c`: guarda enderecos de raizes explicitas, nao apenas o valor
  atual da raiz.
- `src/stack_roots.c`: examina conservadoramente a regiao de pilha suportada no
  ambiente Windows atual.
- `src/register_roots.c`: usa `setjmp()` para capturar uma representacao dos
  registradores e trata o `jmp_buf` como memoria conservadora.
- `src/sweeper.c`: remove ou preserva objetos conforme marca, gera estatisticas
  e promove jovens sobreviventes para a geracao antiga.
- `src/old_pages.c`: rastreia paginas antigas protegiveis, protecao com
  `VirtualProtect()` e estado sujo usado pelo remembered set.

## Arvore de intervalos

Cada objeto gerenciado e representado individualmente na arvore AVL. A arvore
usa `uintptr_t` para comparar enderecos e intervalos semiabertos `[start, end)`.
Insercoes rejeitam sobreposicao. Buscas usam `max_end` para podar subarvores e
encontrar candidatos em `O(log n)` quando a arvore esta balanceada.

As invariantes relevantes sao:

- propriedade de busca por inicio/fim do intervalo;
- ausencia de sobreposicao;
- altura AVL e fator de balanceamento em `[-1, 1]`;
- `max_end` consistente apos insercao, remocao e rotacoes.

Em builds sem `NDEBUG`, o coletor valida invariantes da arvore antes e depois de
coletar.

## Alocacao e heap

O heap gerenciado vem de `VirtualAlloc()`. Objetos pequenos usam arenas de 64
KiB com classes de tamanho de 32, 64, 128, 256, 512 e 1024 bytes. Objetos maiores
usam mapeamentos dedicados. Essa divisao evita um `VirtualAlloc()` por objeto
pequeno, que seria inviavel no teste de fogo.

Os metadados ficam fora da area entregue ao usuario e nao sao alocados com
`gc_malloc()`. Em modo debug, canarios protegem os limites logicos dos objetos.
O sweep devolve blocos pequenos para freelists, reutiliza arenas parcialmente
ocupadas e libera arenas vazias quando seguro.

## Marcacao e sweep

A marcacao e conservadora: qualquer palavra que pareca apontar para o interior
de um objeto gerenciado pode preservar esse objeto. Candidatos desalinhados sao
lidos com `memcpy()`. Isso reduz risco de acesso invalido e evita depender de
alinhamento da plataforma.

A fila de marcacao e iterativa, nao recursiva. Isso evita estouro da pilha em
listas longas, arvores profundas ou grafos ciclicos. O sweep separa objetos
jovens e antigos para suportar coleta menor e maior.

## Geracoes e remembered set

Novas alocacoes entram na geracao jovem. Uma coleta menor marca raizes
explicitas, pilha, registradores e referencias de paginas antigas sujas para
jovens. Em seguida, varre apenas jovens nao marcados.

Objetos jovens promovem para a geracao antiga depois de
`GC_DEFAULT_PROMOTION_THRESHOLD` sobrevivencias, padrao 2. Depois de
`GC_DEFAULT_MAJOR_COLLECTION_INTERVAL` coletas menores, padrao 4, `gc_collect()`
executa uma coleta maior completa sobre jovens e antigos.

Paginas antigas protegiveis sao marcadas como somente leitura com
`VirtualProtect()`. Na primeira escrita valida, o tratador vetorizado registra a
pagina como suja e restaura permissao de escrita. Essas paginas sujas alimentam
o remembered set da proxima coleta menor.

## Testes e validacao

Comandos principais:

```powershell
mingw32-make clean
mingw32-make all
mingw32-make test
mingw32-make sanitize
mingw32-make stress
mingw32-make benchmark
mingw32-make plots
```

`test` executa testes unitarios da arvore, marcador, alocador, paginas antigas,
pilha, registradores, sweep e API publica. Os exemplos de lista, arvore e grafo
ciclico tambem entram nesse alvo.

`sanitize` usa AddressSanitizer e UndefinedBehaviorSanitizer no ambiente Clang64
do MSYS2. Dr. Memory nao esta disponivel no ambiente atual; se instalado, deve
ser tratado como validacao complementar. Nao ha substituto direto perfeito: para
Windows nativo, a combinacao pratica e ASan/UBSan, Application Verifier/PageHeap
e depuracao com WinDbg quando necessario.

## Benchmarks e graficos

Os benchmarks atuais sao:

- `bench_scale_allocations`: mede escala de alocacao, pausa, memoria reservada,
  bytes vivos e coletados;
- `bench_fire_test`: cria ciclos e referencias cruzadas deterministicas para
  testar sobrevivencia e recuperacao;
- `bench_tree`: mede insercao, busca e remocao da arvore com sementes fixas;
- `bench_compare_collectors`: compara a mesma carga em mark-sweep puro e
  coletor geracional.

O alvo `plots` gera CSVs em `data/` e SVGs em `plots/` usando
`scripts/generate_plots.py`. Os valores dos graficos saem dos executaveis de
benchmark, sem edicao manual.

## Limitacoes conhecidas

- A analise conservadora pode reter lixo se um inteiro parecer ponteiro valido.
  Isso e falso positivo, nao coleta incorreta.
- Testes de pilha e registradores dependem de ABI, otimizacao e comportamento do
  compilador. O projeto e validado no ambiente Windows/MinGW atual.
- A barreira com `VirtualProtect()` cobre paginas antigas protegiveis. Objetos
  antigos pequenos em arenas ainda podem dividir pagina com jovens, blocos
  livres e estruturas internas, entao sao tratados conservadoramente.
- O tratador de excecao vetorizado deve permanecer minimo e reentrante. Ele nao
  deve alocar memoria nem fazer operacoes complexas.
- Em builds com AddressSanitizer, a protecao de pagina da barreira e desativada
  para evitar conflito com SEH e shadow memory do runtime do sanitizador.
- O projeto ainda nao oferece suporte multithread.

## Relacao com Estruturas de Dados e Sistemas Operacionais

Em Estruturas de Dados, o ponto central e a arvore AVL de intervalos: ela mantem
altura logaritmica, rejeita sobreposicoes e usa `max_end` para localizar
ponteiros interiores de forma eficiente.

Em Sistemas Operacionais, o projeto exercita memoria virtual com `VirtualAlloc`,
protecoes de pagina com `VirtualProtect`, tratamento de excecoes de acesso,
varredura conservadora de pilha/registradores e medicao de pausas com
`QueryPerformanceCounter`.

As decisoes de engenharia privilegiam corretude e rastreabilidade: testes de
invariantes, canarios, sanitizadores, benchmarks reproduziveis e diario de
engenharia acompanham cada marco.
