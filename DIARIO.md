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
