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

## 2026-06-20 16:46 — Busca por ponteiro interior

- Prompt/objetivo: prosseguir para o Commit 7 da Fase 1.
- Fase do PLAN.md: Fase 1 — Árvore de intervalos; Commit 7.
- Arquivos examinados: `SKILL.md`, `PLAN.md`, `DIARIO.md`, `README.md`, cabeçalho, implementação e testes da árvore de intervalos, estado e histórico Git.
- Alterações realizadas: adição da busca pontual iterativa, variante instrumentada com contagem de nós examinados, testes de limites, ponteiros interiores, endereços externos e poda por `max_end`.
- Decisões e justificativas: a API comum não expõe instrumentação; a variante `interval_tree_find_counted()` atende testes e benchmarks; como `max_end` é exclusivo, a subárvore esquerda só pode conter o endereço quando `address < left->max_end`; quando o endereço está antes da raiz e a esquerda é podada, a busca termina porque todos os nós da direita começam depois da raiz; a busca é iterativa para manter uso constante de pilha.
- Riscos ou erros procurados: tratar `end` como inclusivo, perder o último byte válido, falhar em `UINTPTR_MAX`, percorrer subárvore podada, contagem incorreta em árvore vazia e complexidade maior que a altura da AVL.
- Testes executados: `mingw32-make clean`, `mingw32-make all`, `mingw32-make test`, `mingw32-make sanitize`, `mingw32-make stress`, teste específico de `test_interval_tree` e `git diff --check`.
- Resultados: compilação sem warnings; testes normais e ASan/UBSan passaram; início, interior e último byte retornaram o nó correto; limite final e lacunas retornaram `NULL`; em uma árvore controlada de 31 nós, todas as buscas examinaram no máximo a altura da árvore; a poda de uma subárvore esquerda foi confirmada com uma única visita.
- Erros da IA ou sugestões rejeitadas: o primeiro fixture criado para demonstrar a poda tinha fator AVL igual a dois; a revisão adicionou a subárvore direita necessária e manteve a demonstração em uma árvore balanceada.
- Pendências e próximo passo: revisar o diff, criar o Commit 7 após autorização explícita e então implementar a remoção AVL do Commit 8.

## 2026-06-20 17:12 — Remoção AVL por endereço inicial

- Prompt/objetivo: prosseguir para o Commit 8 da Fase 1.
- Fase do PLAN.md: Fase 1 — Árvore de intervalos; Commit 8.
- Arquivos examinados: `SKILL.md`, `PLAN.md`, `DIARIO.md`, `README.md`, cabeçalho, implementação e testes da árvore de intervalos, estado e histórico Git.
- Alterações realizadas: adição da remoção por endereço inicial, extração do sucessor mínimo, transplante de nó, destaque do nó removido, atualização de metadados, rebalanceamento e testes dos casos exigidos.
- Decisões e justificativas: a API devolve opcionalmente o nó removido já destacado; em nós com dois filhos, o sucessor real é transplantado em vez de copiar seus limites, preservando a identidade do metadado associado ao objeto; a extração do mínimo rebalanceia o caminho de retorno antes do transplante.
- Riscos ou erros procurados: perda do filho direito do sucessor, cópia que altere a identidade dos nós, ponteiros residuais no nó removido, falha que modifique a árvore, `max_end` que não diminua, raiz não substituída e desequilíbrio após remoções sucessivas.
- Testes executados: `mingw32-make clean`, `mingw32-make all`, `mingw32-make test`, `mingw32-make sanitize`, `mingw32-make stress`, teste específico de `test_interval_tree` e `git diff --check`.
- Resultados: compilação sem warnings; testes normais e ASan/UBSan passaram; foram cobertos folha, nó com um filho, raiz com dois filhos e sucessor com filho direito, raiz única, endereço ausente, saída opcional e rebalanceamento com redução de `max_end`.
- Erros da IA ou sugestões rejeitadas: nenhum identificado.
- Pendências e próximo passo: revisar o diff, criar o Commit 8 após autorização explícita e então implementar o verificador público de invariantes do Commit 9.

## 2026-06-20 21:23 — Verificação pública das invariantes AVL

- Prompt/objetivo: prosseguir para o Commit 9 da Fase 1.
- Fase do PLAN.md: Fase 1 — Árvore de intervalos; Commit 9.
- Arquivos examinados: `SKILL.md`, `PLAN.md`, `DIARIO.md`, `README.md`, `Makefile`, cabeçalho, implementação e testes da árvore de intervalos, estado e histórico Git.
- Alterações realizadas: adição de `interval_tree_validate()` para builds sem `NDEBUG`, testes de corrupção controlada e sequência pseudoaleatória determinística de inserções, buscas e remoções.
- Decisões e justificativas: o verificador percorre a árvore em ordem, recalcula altura e `max_end` e não modifica os nós; a ordem crescente de inícios e o limite final do predecessor verificam simultaneamente a propriedade de busca e a ausência de sobreposição; a semente fixa torna o teste aleatório reproduzível.
- Riscos ou erros procurados: intervalos inválidos ou sobrepostos, ordenação incorreta, altura armazenada divergente, fator AVL fora de `[-1, 1]`, `max_end` corrompido e falhas intermitentes em sequências mistas.
- Testes executados: `mingw32-make clean`, `mingw32-make all`, teste específico `build\test_interval_tree.exe`, `mingw32-make test`, `mingw32-make sanitize`, `mingw32-make stress`, compilação isolada de `src\interval_tree.c` com `-DNDEBUG`, consulta da disponibilidade do Dr. Memory e `git diff --check`.
- Resultados: compilação sem warnings; testes normais, stress e ASan/UBSan passaram; 2.048 operações pseudoaleatórias validaram a árvore após cada inserção ou remoção; corrupções de ordenação, sobreposição, altura, balanceamento e `max_end` foram detectadas; a compilação com `NDEBUG` passou sem código ou declaração residual do verificador; Dr. Memory não está instalado no ambiente.
- Erros da IA ou sugestões rejeitadas: nenhum identificado.
- Pendências e próximo passo: revisar o diff, criar o Commit 9 somente após autorização explícita e então iniciar a inicialização e o encerramento do coletor no Commit 10.

## 2026-06-21 00:35 — Pedido extra: preparação do Dr. Memory

- Prompt/objetivo: por pedido extra do desenvolvedor, instalar e validar antecipadamente o Dr. Memory para uso nas fases futuras do coletor.
- Fase do PLAN.md: apoio ao Commit 9 e preparação para as validações dinâmicas dos commits posteriores.
- Arquivos examinados: documentação e pacote oficiais do Dr. Memory, issues oficiais `DynamoRIO/drmemory#2538`, `DynamoRIO/drmemory#2539` e `DynamoRIO/dynamorio#6962`, ambiente do usuário, build e teste da árvore.
- Alterações realizadas: remoção do ZIP incompleto deixado por uma interrupção, instalação limpa do Dr. Memory 2.6.0 em `C:\Users\giova\Tools\DrMemory-Windows-2.6.0` e configuração do frontend `bin64` no `PATH` do usuário.
- Decisões e justificativas: o pacote oficial AMD64 foi validado por tamanho, estrutura ZIP e SHA-256 `07632AB77856579D06E30DE344C669E9BDCBDE9688B6EF43E84D31C39C6E46B2`; o frontend x86-64 foi escolhido por ser a plataforma exclusiva do projeto.
- Riscos ou erros procurados: instalação parcial, ZIP corrompido, alteração indevida do repositório, frontend de arquitetura incorreta, falha do programa-alvo e incompatibilidade do instrumentador com Windows 11.
- Testes executados: `drmemory.exe -version`, execução direta do teste da árvore compilado com símbolos e frame pointers, Dr. Memory normal, `-light`, modo `-leaks_only -no_count_leaks -no_track_allocs` e DynamoRIO puro.
- Resultados: instalação e `PATH` estão íntegros; Dr. Memory informa versão 2.6.0; o teste passa diretamente e sob DynamoRIO puro, mas o cliente Dr. Memory não inicia em Windows 11 25H2 por `dbghelp.dll: library initializer failed`, defeito upstream aberto também reproduzido em Windows 11 24H2; portanto a ferramenta ainda não pode ser considerada funcional neste ambiente.
- Erros da IA ou sugestões rejeitadas: a primeira tentativa interrompida deixou um ZIP sem diretório central e foi descartada; a configuração inicial apontou para `bin` e foi corrigida para `bin64`; não foi adotada substituição não suportada de DLLs do sistema.
- Pendências e próximo passo: manter ASan/UBSan como validação dinâmica disponível; repetir o Dr. Memory quando houver correção upstream ou em uma máquina virtual Windows 10 compatível e somente então registrar uma execução bem-sucedida.

## 2026-06-21 00:43 — Retirada do Dr. Memory

- Prompt/objetivo: retirar o Dr. Memory do ambiente do projeto e criar o Commit 9.
- Fase do PLAN.md: Fase 1 — Árvore de intervalos; conclusão do Commit 9.
- Arquivos examinados: instalação em `C:\Users\giova\Tools`, `PATH` do usuário, diretórios de logs, referências no repositório, diff e estado Git.
- Alterações realizadas: remoção completa do Dr. Memory 2.6.0, de seus logs e da entrada correspondente no `PATH`; nenhuma integração de código ou build precisou ser removida.
- Decisões e justificativas: a ferramenta foi retirada porque o cliente não é compatível com o Windows 11 25H2 atual; o registro cronológico da tentativa foi preservado para manter rastreabilidade, enquanto ASan/UBSan continuam sendo as ferramentas dinâmicas efetivamente disponíveis.
- Riscos ou erros procurados: remoção de diretório incorreto, permanência de binários ou logs, alteração indevida de outras entradas do `PATH` e referências técnicas que fizessem a build depender da ferramenta.
- Testes executados: verificação dos caminhos antes e depois da remoção, busca por referências no repositório e validação final do Commit 9.
- Resultados: instalação, logs e entrada de `PATH` removidos; o projeto nunca teve dependência de Dr. Memory e permanece funcional sem ele.
- Erros da IA ou sugestões rejeitadas: nenhum identificado.
- Pendências e próximo passo: criar o Commit 9 e então iniciar a inicialização e o encerramento do coletor no Commit 10.

## 2026-06-21 00:51 — Inicialização do estado do coletor

- Prompt/objetivo: prosseguir para o Commit 10 da Fase 2.
- Fase do PLAN.md: Fase 2 — Heap gerenciado e API mínima; Commit 10.
- Arquivos examinados: `SKILL.md`, `PLAN.md`, `DIARIO.md`, `README.md`, `Makefile`, infraestrutura de testes, documentação oficial Win32 e estado Git.
- Alterações realizadas: criação da API inicial do coletor, estado global interno, captura dos limites da pilha, vínculo com a thread proprietária, encerramento do estado, consulta de status, teste específico e integração aos builds normal e sanitizado.
- Decisões e justificativas: `GetCurrentThreadStackLimits()` fornece os limites alocados pelo Windows e exige `_WIN32_WINNT >= 0x0602`; o projeto define Windows 10 como alvo de cabeçalho por suportar apenas Windows 11; `gc_shutdown()` mantém a assinatura `void` do plano e comunica erros por `gc_get_status()`; chamadas de outra thread são rejeitadas porque o coletor permanece explicitamente monothread.
- Riscos ou erros procurados: limites invertidos, endereço atual fora da pilha capturada, inicialização repetida, encerramento sem inicialização, encerramento pela thread errada, estado residual após shutdown e vazamento do handle usado no teste.
- Testes executados: `mingw32-make clean`, `mingw32-make all`, teste específico `build\test_gc.exe`, `mingw32-make test`, `mingw32-make sanitize`, `mingw32-make stress`, inspeção PE/imports com `objdump` e `git diff --check`.
- Resultados: compilação GCC e Clang sem warnings; testes normais, stress e ASan/UBSan passaram; o executável foi confirmado como PE x86-64 e importa as APIs Win32 esperadas; limites válidos contiveram uma variável local; inicialização duplicada, argumento inválido, shutdown sem inicialização e shutdown por outra thread foram detectados; o estado e os limites foram zerados no encerramento.
- Erros da IA ou sugestões rejeitadas: nenhum identificado.
- Pendências e próximo passo: revisar o diff, criar o Commit 10 somente após autorização explícita e então implementar o protótipo de alocação com `VirtualAlloc()` no Commit 11.

## 2026-06-21 01:10 — Estudo de visualização no terminal

- Prompt/objetivo: avaliar uma maneira visual de acompanhar o comportamento da árvore e do coletor pelo terminal.
- Fase do PLAN.md: análise complementar após o Commit 10, sem iniciar o Commit 11.
- Arquivos examinados: APIs públicas e internas atuais da árvore de intervalos e do estado do coletor, estado e histórico Git.
- Alterações realizadas: nenhuma alteração de código; definição preliminar de uma visualização ASCII de snapshots e eventos de depuração.
- Decisões e justificativas: a visualização deve ficar em módulo de diagnóstico separado, receber um `FILE *` e não alterar a árvore nem o coletor; snapshots da árvore devem mostrar intervalo, altura, fator AVL e `max_end`; snapshots do coletor devem mostrar estado, pilha e, quando existirem, objetos, marcação e gerações; cores e animação devem ser opcionais para preservar logs reproduzíveis.
- Riscos ou erros procurados: poluir a API pública, modificar estado durante a inspeção, depender obrigatoriamente de sequências ANSI, usar recursão sobre estruturas potencialmente grandes e misturar instrumentação com o caminho de produção.
- Testes executados: inspeção estática das interfaces atuais; nenhum build foi necessário porque não houve alteração de código.
- Resultados: a visualização é viável no terminal e pode ser adicionada como ferramenta de depuração independente, começando pela árvore e evoluindo junto com o coletor.
- Erros da IA ou sugestões rejeitadas: nenhum identificado.
- Pendências e próximo passo: decidir se a ferramenta ASCII será implementada como commit complementar antes do Commit 11 ou após existir a primeira alocação gerenciada.

## 2026-06-21 01:21 — Pedido extra: visualizador interativo da árvore

- Prompt/objetivo: criar uma visualização puramente da árvore, com menu interativo e valores gerados automaticamente, além de script PowerShell para compilar e executar em debug.
- Fase do PLAN.md: marco complementar entre os Commits 10 e 11, sem alterar o coletor.
- Arquivos examinados: `SKILL.md`, `PLAN.md`, `DIARIO.md`, `README.md`, API e implementação da árvore, diretórios `examples/` e `scripts/`, build e estado Git.
- Alterações realizadas: criação do visualizador ASCII, menu por teclas para inserção, remoção, busca, nova árvore, animação e validação, modo automático `--demo`, script PowerShell com `-Demo` e `-BuildOnly` e documentação de uso.
- Decisões e justificativas: intervalos são gerados dentro de faixas independentes para evitar sobreposição por construção; a ferramenta reutiliza somente a API pública da árvore; o modo automático permite validação reproduzível do executável sem entrada interativa; a compilação usa C11 estrito, `-g3`, `-O0` e frame pointers.
- Riscos ou erros procurados: reutilização de nó ainda conectado, sobreposição aleatória, remoção de identidade incorreta, invariantes AVL após operações mistas, warnings, dependência de entrada numérica e artefatos fora de `build/`.
- Testes executados: script com `-BuildOnly` e `-Demo`, 25 execuções automáticas, compilação e execução com ASan/UBSan, `mingw32-make clean`, `all`, `test`, `sanitize`, `stress` e `git diff --check`.
- Resultados: visualização e script compilaram sem warnings; todas as execuções mantiveram as invariantes; suíte normal, stress e ASan/UBSan passaram; nenhuma entrada de intervalo ou endereço é solicitada ao usuário.
- Erros da IA ou sugestões rejeitadas: o primeiro comando combinado de sanitização foi rejeitado pelo parser do PowerShell por causa da vírgula da flag e foi repetido com argumentos corretamente delimitados; nenhum código chegou a executar nessa tentativa.
- Pendências e próximo passo: revisar o diff, criar um commit complementar somente após autorização explícita e então retomar o Commit 11.
