# Diário de engenharia

Este arquivo registra, em ordem cronológica, decisões, testes, riscos e
pendências do desenvolvimento. Entradas anteriores não devem ser reescritas.

## 2026-06-19 18:39 — Fundação do repositório

- Prompt/objetivo: Ler `SKILL.md` e `PLAN.md` e iniciar o Commit 1 da seção 5.
- Fase do PLAN.md: Fase 0 — Fundação e rastreabilidade; Commit 1.
- Arquivos examinados: `SKILL.md`, `PLAN.md` e conteúdo da raiz do projeto.
- Alterações realizadas: criação de `.gitignore`, `README.md`, `DIARIO.md` e da estrutura mínima de diretórios por meio de arquivos `.gitkeep`.
- Decisões e justificativas: nenhuma implementação ou sistema de build foi adicionado, mantendo o objetivo lógico exclusivo do Commit 1; a plataforma inicial foi limitada a Linux x86-64 e uma thread; por solicitação explícita do usuário, `PLAN.md` e `SKILL.md` foram mantidos como instruções locais ignoradas pelo Git.
- Riscos ou erros procurados: inclusão indevida de artefatos, mistura com o escopo do Commit 2, documentação incompatível com os requisitos e perda de arquivos preexistentes.
- Testes executados: inspeção do estado inicial, contagem de linhas, `git diff --check`, `make clean`, `make all` e `make test`.
- Resultados: estrutura e documentação inicial criadas sem código de produção; `git diff --check` terminou com código zero; os três comandos `make` não foram executados porque a ferramenta não está instalada no ambiente Windows atual e o `Makefile` pertence ao Commit 2.
- Erros da IA ou sugestões rejeitadas: nenhum identificado.
- Pendências e próximo passo: criar o sistema de build e o teste mínimo previstos no Commit 2.

## 2026-06-19 19:27 — Sistema de build inicial

- Prompt/objetivo: iniciar o Commit 2 da seção 5 do `PLAN.md`.
- Fase do PLAN.md: Fase 0 — Fundação e rastreabilidade; Commit 2.
- Arquivos examinados: `SKILL.md`, `PLAN.md`, `DIARIO.md`, `README.md`, `.gitignore`, histórico Git e ferramentas disponíveis no ambiente.
- Alterações realizadas: criação de `Makefile`, criação de `tests/test_smoke.c` e atualização do estado e das instruções de build no `README.md`.
- Decisões e justificativas: o executável mínimo valida somente a cadeia de compilação C11; macros de asserção, separação da suíte e sanitizadores permanecem no Commit 3; o `Makefile` diferencia apenas comandos de diretório e extensão do executável no Windows, preservando o fluxo principal destinado a Linux.
- Riscos ou erros procurados: warnings ocultos, flags incompletas, alvos não declarados como phony, artefatos fora de `build/`, comandos de limpeza destrutivos e antecipação indevida do Commit 3.
- Testes executados: `mingw32-make clean`, `mingw32-make all`, `mingw32-make test`, `mingw32-make stress`, simulação do ramo Linux com `mingw32-make --dry-run clean all test` e `git diff --check`.
- Resultados: compilação e execução terminaram com código zero usando GCC 15.2.0 e GNU Make 4.4.1 para `x86_64-w64-mingw32`, sem warnings; a simulação gerou os comandos esperados para Linux; não há distribuição WSL instalada, portanto `make clean`, `make all` e `make test` ainda precisam ser confirmados em Linux x86-64.
- Erros da IA ou sugestões rejeitadas: nenhum identificado.
- Pendências e próximo passo: validar o Commit 2 em Linux x86-64, criar o commit após autorização do usuário e então iniciar a infraestrutura de testes do Commit 3.

## 2026-06-19 19:40 — Revisão para ambiente Windows

- Prompt/objetivo: dispensar a validação Linux do Commit 2, registrar a mudança do ambiente para Windows, revisar compatibilidade e criar o commit.
- Fase do PLAN.md: Fase 0 — Fundação e rastreabilidade; revisão final do Commit 2.
- Arquivos examinados: `SKILL.md`, `PLAN.md`, `DIARIO.md`, `README.md`, `Makefile`, `tests/test_smoke.c`, estado e histórico Git.
- Alterações realizadas: documentação atualizada para Windows 11 x86-64 com MSYS2/MinGW-w64; `Makefile` simplificado para comandos nativos do Windows; limitação da validação Linux registrada; instruções locais sincronizadas com APIs Win32 equivalentes.
- Decisões e justificativas: a validação em Linux foi dispensada por autorização explícita do usuário e por ausência de distribuição WSL; o ambiente e a plataforma inicial do projeto passam a ser Windows 11 x86-64; o teste mínimo usa somente C11 padrão.
- Riscos ou erros procurados: uso acidental de comandos POSIX no build, extensão incorreta do executável, APIs indisponíveis no Windows e alegações de testes não executados.
- Testes executados: revisão estática de todas as fontes e receitas do build, seguida de `mingw32-make clean`, `mingw32-make all`, `mingw32-make test`, `mingw32-make stress` e `git diff --check`.
- Resultados: nenhum código atual depende de APIs Linux; `stdio.h`, `stdlib.h`, `puts()` e `EXIT_SUCCESS` são recursos C11 compatíveis; referências futuras foram ajustadas para `VirtualAlloc`, `VirtualFree`, `VirtualProtect`, tratamento vetorizado de exceções e `QueryPerformanceCounter`.
- Erros da IA ou sugestões rejeitadas: a orientação anterior tratava Linux como plataforma principal; ela foi substituída pela decisão explícita de desenvolver para Windows.
- Pendências e próximo passo: no Commit 3, criar a infraestrutura de testes para Windows; nos marcos de memória virtual, validar cuidadosamente granularidade de páginas, flags de proteção, códigos de erro e semântica do tratador de exceção.

## 2026-06-19 20:06 — Infraestrutura de testes e sanitizadores

- Prompt/objetivo: prosseguir para o Commit 3 da Fase 0.
- Fase do PLAN.md: Fase 0 — Fundação e rastreabilidade; Commit 3.
- Arquivos examinados: `SKILL.md`, `PLAN.md`, `DIARIO.md`, `README.md`, `Makefile`, `tests/test_smoke.c`, estado e histórico Git, compiladores e pacotes MSYS2 disponíveis.
- Alterações realizadas: criação de `tests/test.h` com macros de asserção, criação de `tests/test_assertions.c`, adaptação do teste smoke, separação dos executáveis de teste por módulo e adição do alvo `sanitize` com ASan e UBSan.
- Decisões e justificativas: o build normal permanece em GCC/MinGW; a sanitização usa a cadeia `clang64` completa do MSYS2 porque o GCC/MinGW instalado não fornece `libasan` nem `libubsan`; cada expressão das asserções de igualdade é avaliada uma única vez.
- Riscos ou erros procurados: dupla avaliação em macros, falhas retornando código zero, mistura incompatível entre linkers `mingw64` e `clang64`, ausência das DLLs de runtime, caminhos com barras incompatíveis com `cmd.exe` e artefatos não removidos.
- Testes executados: prova isolada de suporte a GCC/MinGW, instalação e inspeção dos pacotes MSYS2, `mingw32-make clean`, `mingw32-make all`, `mingw32-make test`, `mingw32-make sanitize`, `mingw32-make stress` e limpeza final.
- Resultados: testes normais passaram sem warnings; testes ASan/UBSan compilaram e executaram com Clang 21.1.8; o autoteste confirmou que uma asserção malsucedida retorna `EXIT_FAILURE`, diferente de zero; `build/` foi removido ao final.
- Erros da IA ou sugestões rejeitadas: a tentativa inicial de usar o pacote Clang do ambiente `mingw64` foi incorreta porque ele não inclui runtimes de sanitização compatíveis; a mistura manual de runtimes `clang64` com o linker `mingw64` também foi rejeitada após erros de ligação; a solução foi usar integralmente a cadeia `clang64` no alvo auxiliar.
- Pendências e próximo passo: revisar o diff, criar o Commit 3 após autorização explícita e iniciar o Commit 4 com a definição do nó da árvore de intervalos.

## 2026-06-20 16:03 — Nó básico da árvore de intervalos

- Prompt/objetivo: começar o Commit 4 da Fase 1.
- Fase do PLAN.md: Fase 1 — Árvore de intervalos; Commit 4.
- Arquivos examinados: `SKILL.md`, `PLAN.md`, `DIARIO.md`, `README.md`, `Makefile`, infraestrutura de testes e histórico Git.
- Alterações realizadas: criação de `IntervalNode`, auxiliares de altura e `max_end`, atualização dos metadados do nó, testes unitários e integração do novo módulo aos builds normal e sanitizado.
- Decisões e justificativas: intervalos são semiabertos `[start, end)`, altura nula é zero e altura de folha é um; `max_end` armazena o maior limite final exclusivo da subárvore; intervalos vazios, invertidos e nó nulo são rejeitados na inicialização.
- Riscos ou erros procurados: comparação incorreta de endereços, overflow no limite `UINTPTR_MAX`, inconsistência entre limites inclusivos e exclusivos, altura incorreta, `max_end` desatualizado e dereferência de filho nulo.
- Testes executados: `mingw32-make clean`, `mingw32-make all`, `mingw32-make test`, `mingw32-make sanitize`, `mingw32-make stress`, teste específico de `test_interval_tree` e `git diff --check`.
- Resultados: compilação sem warnings; testes normais e ASan/UBSan passaram; foram cobertos nós unitários, intervalos inválidos, limites zero e `UINTPTR_MAX`, sentinelas nulas e propagação de altura e `max_end`.
- Erros da IA ou sugestões rejeitadas: nenhum identificado.
- Pendências e próximo passo: revisar o diff, criar o Commit 4 após autorização explícita e então implementar as rotações AVL do Commit 5.

## 2026-06-20 16:13 — Dependência das rotações AVL

- Prompt/objetivo: prosseguir diretamente para o Commit 6 da Fase 1.
- Fase do PLAN.md: Fase 1 — Árvore de intervalos; avaliação do Commit 6.
- Arquivos examinados: `SKILL.md`, `PLAN.md`, `DIARIO.md`, cabeçalho, implementação e testes da árvore de intervalos, estado e histórico Git.
- Alterações realizadas: nenhuma alteração de código; somente análise da dependência entre os marcos.
- Decisões e justificativas: o Commit 6 não foi iniciado porque exige rebalanceamento após inserções, mas as rotações simples e duplas necessárias pertencem ao Commit 5 e ainda não existiam.
- Riscos ou erros procurados: inserção que degrade para árvore linear, alturas incorretas, `max_end` desatualizado, rebalanceamento incompleto e histórico fora da ordem incremental.
- Testes executados: inspeção do código e do histórico; nenhum teste de build foi necessário porque não houve alteração de código.
- Resultados: dependência técnica confirmada; a solicitação foi corrigida pelo usuário para retomar o Commit 5.
- Erros da IA ou sugestões rejeitadas: foi rejeitada a implementação de inserção sem rotações ou a combinação dos Commits 5 e 6 em uma única mudança.
- Pendências e próximo passo: implementar e validar as rotações AVL do Commit 5.

## 2026-06-20 16:17 — Rotações AVL

- Prompt/objetivo: corrigir a sequência solicitada e realizar o Commit 5 da Fase 1.
- Fase do PLAN.md: Fase 1 — Árvore de intervalos; Commit 5.
- Arquivos examinados: `SKILL.md`, `PLAN.md`, `DIARIO.md`, `README.md`, cabeçalho, implementação e testes da árvore de intervalos, estado e histórico Git.
- Alterações realizadas: adição do fator de balanceamento, rotações simples à esquerda e à direita, rotações duplas esquerda-direita e direita-esquerda, testes e atualização do estado no README.
- Decisões e justificativas: as rotações retornam a nova raiz da subárvore e não modificam a estrutura quando faltam os filhos exigidos; nas rotações simples, a variável `pivot` identifica explicitamente o filho que será promovido, mantém acessível a subárvore que precisa ser transferida e torna a troca de ponteiros e a nova raiz menos sujeitas a confusão; o nó rebaixado é atualizado antes do `pivot` promovido para preservar altura e `max_end` de baixo para cima.
- Riscos ou erros procurados: perda da subárvore de transferência, inversão da ordenação, atualização de metadados na ordem errada, fator de balanceamento incorreto, acesso nulo e alteração parcial quando a rotação não é aplicável.
- Testes executados: `mingw32-make clean`, `mingw32-make all`, `mingw32-make test`, `mingw32-make sanitize`, `mingw32-make stress`, teste específico de `test_interval_tree` e `git diff --check`.
- Resultados: compilação sem warnings; testes normais e ASan/UBSan passaram; rotações simples e duplas preservaram ordenação sem sobreposição, subárvores de transferência, alturas, fatores de balanceamento e `max_end`.
- Erros da IA ou sugestões rejeitadas: os primeiros dados dos testes de transferência continham intervalos sobrepostos; a revisão detectou e corrigiu os limites antes da entrega; o pedido anterior de pular o Commit 5 permaneceu registrado como rejeitado por dependência técnica.
- Pendências e próximo passo: revisar o diff, criar o Commit 5 após autorização explícita e então implementar a inserção AVL do Commit 6.

## 2026-06-20 16:36 — Inserção AVL de intervalos

- Prompt/objetivo: prosseguir para o Commit 6 da Fase 1.
- Fase do PLAN.md: Fase 1 — Árvore de intervalos; Commit 6.
- Arquivos examinados: `SKILL.md`, `PLAN.md`, `DIARIO.md`, `README.md`, cabeçalho, implementação e testes da árvore de intervalos, estado e histórico Git.
- Alterações realizadas: adição da API de inserção, detecção de sobreposição, atualização de metadados, rebalanceamento AVL e testes com sequências crescentes, decrescentes e pseudoaleatórias determinísticas.
- Decisões e justificativas: a árvore não aloca metadados; o chamador fornece um nó válido e destacado; a API recebe ponteiro para a raiz para poder substituí-la após rotações; para intervalos semiabertos, a inserção segue à esquerda quando `node->end <= root->start`, à direita quando `node->start >= root->end` e rejeita os demais casos como sobreposição.
- Riscos ou erros procurados: aceitação de sobreposição parcial, contenção ou duplicata; alteração da árvore após falha; perda da raiz após rotação; metadados desatualizados; dupla avaliação de limites; degradação de sequências ordenadas; inserção de nó já conectado.
- Testes executados: `mingw32-make clean`, `mingw32-make all`, `mingw32-make test`, `mingw32-make sanitize`, `mingw32-make stress`, teste específico de `test_interval_tree` e `git diff --check`.
- Resultados: compilação sem warnings; testes normais e ASan/UBSan passaram; um verificador interno de teste recalculou ordenação sem sobreposição, altura, fator AVL e `max_end` após cada sequência; adjacência foi aceita e sobreposições parciais, contidas, abrangentes e duplicadas foram rejeitadas sem mudança estrutural.
- Erros da IA ou sugestões rejeitadas: nenhum identificado.
- Pendências e próximo passo: revisar o diff, criar o Commit 6 após autorização explícita e então implementar a busca pontual do Commit 7.
