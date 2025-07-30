## MiniFS: Um Sistema de Arquivos Virtual em C
![alt text](https://img.shields.io/badge/Language-C-blue.svg)

![alt text](https://img.shields.io/badge/Language-Python-orange.svg)


MiniFS é um sistema de arquivos virtual implementado em C, projetado como uma ferramenta educacional para desvendar o funcionamento interno de sistemas de arquivos baseados em árvore, como os onipresentes em sistemas operacionais UNIX-like (Linux, macOS). Ele opera inteiramente na memória RAM, simulando a lógica de um sistema de arquivos real, mas inclui funcionalidades essenciais para a persistência de dados em disco e uma interface de linha de comando (shell) interativa para uma experiência de usuário completa.
Para complementar a experiência de aprendizado, o projeto inclui um script em Python que gera uma visualização gráfica da estrutura de diretórios diretamente no terminal, transformando a abstrata estrutura de dados em uma representação visual e intuitiva.

### 📜 Índice
1.  [Introdução: Desmistificando a Caixa Preta](#1-introdução-desmistificando-a-caixa-preta)
2.  [Conceitos Fundamentais: A Arquitetura em Árvore](#2-conceitos-fundamentais-a-arquitetura-em-árvore)
3.  [Funcionalidades Principais: O Kit de Ferramentas do Usuário](#3-funcionalidades-principais-o-kit-de-ferramentas-do-usuário)
4.  [Estrutura do Projeto: Um Design Modular e Limpo](#4-estrutura-do-projeto-um-design-modular-e-limpo)
5.  [Análise Detalhada do Código-Fonte: Mergulhando na Implementação](#5-análise-detalhada-do-código-fonte-mergulhando-na-implementação)
    *   [A Estrutura de Dados Node: O DNA do Sistema](#a-estrutura-de-dados-node-o-dna-do-sistema)
    *   [fs.c & fs.h: O Coração Lógico do Sistema](#fsc--fsh-o-coração-lógico-do-sistema)
    *   [shell.c & shell.h: A Interface com o Usuário](#shellc--shellh-a-interface-com-o-usuário)
    *   [main.c: O Ciclo de Vida da Aplicação](#mainc-o-ciclo-de-vida-da-aplicação)
    *   [utils.c & utils.h: Funções de Apoio Essenciais](#utilsc--utilsh-funções-de-apoio-essenciais)
    *   [visualize.py: Tornando o Invisível, Visível](#visualizepy-tornando-o-invisível-visível)
6.  [Como Compilar e Usar: Do Código-Fonte ao Shell Interativo](#6-como-compilar-e-usar-do-código-fonte-ao-shell-interativo)
    *   [Pré-requisitos](#pré-requisitos)
    *   [Compilação Detalhada](#compilação-detalhada)
    *   [Execução](#execução)
    *   [Guia de Comandos Completo](#guia-de-comandos-completo)
7.  [Guia Prático: Uma Sessão de Teste Completa](#7-guia-prático-uma-sessão-de-teste-completa)
    *   [Passo 1: Compilação e Primeira Execução](#passo-1-compilação-e-primeira-execução)
    *   [Passo 2: Construindo a Estrutura de Arquivos](#passo-2-construindo-a-estrutura-de-arquivos)
    *   [Passo 3: Editando Conteúdo e Movendo Arquivos](#passo-3-editando-conteúdo-e-movendo-arquivos)
    *   [Passo 4: Salvando o Estado e Verificando a Persistência](#passo-4-salvando-o-estado-e-verificando-a-persistência)
    *   [Passo 5: Visualizando a Árvore de Arquivos](#passo-5-visualizando-a-árvore-de-arquivos)
8.  [Comparação Aprofundada com FAT (MS-DOS): Abstração vs. Realidade Física](#8-comparação-aprofundada-com-fat-ms-dos-abstração-vs-realidade-física)
    *   [Tabela Comparativa Expandida](#tabela-comparativa-expandida)
    *   [Diferenças Conceituais e Técnicas Cruciais](#diferenças-conceituais-e-técnicas-cruciais)
9.  [Vantagens e Proposta Educacional: Mais que um Projeto, um Laboratório](#9-vantagens-e-proposta-educacional-mais-que-um-projeto-um-laboratório)
10. [Limitações do Projeto: Escolhas de Design Deliberadas](#10-limitações-do-projeto-escolhas-de-design-deliberadas)
11. [Conclusão: Uma Ponte entre a Teoria e a Prática](#11-conclusão-uma-ponte-entre-a-teoria-e-a-prática)

### 1. Introdução: Desmistificando a Caixa Preta
Para a maioria dos usuários e até mesmo muitos desenvolvedores, o sistema de arquivos é uma "caixa preta". Sabemos que podemos criar pastas, salvar arquivos e navegar por caminhos, mas os mecanismos internos que orquestram essas operações permanecem um mistério. O MiniFS foi criado com a missão de abrir essa caixa preta. Para estudantes de Sistemas Operacionais, ele serve como um laboratório prático e interativo para explorar conceitos que são a base dos sistemas modernos:
*   **Estruturas de Dados em Árvore:** Como uma hierarquia de diretórios é representada eficientemente na memória.
*   **Manipulação de Nós:** A lógica por trás da criação (`mkdir`, `touch`), exclusão (`rm`), e movimentação (`mv`) de arquivos e pastas.
*   **Resolução de Caminhos (Path Resolution):** O algoritmo que traduz um caminho como `/home/user/doc` em uma série de ponteiros para nós na árvore.
*   **Serialização de Dados:** O processo de "congelar" uma estrutura de dados complexa da RAM em um formato linear em disco (persistência) e o processo inverso de "descongelá-la" de volta.
*   **Implementação de Shell:** O design de uma interface de linha de comando (REPL: Read-Eval-Print Loop) que interpreta comandos do usuário e os traduz em chamadas de API.

Diferente de sistemas reais como o FAT ou o EXT4, que interagem com o hardware de armazenamento em baixo nível (setores, blocos, cilindros), o MiniFS abstrai essa complexidade. Ele constrói sua estrutura de dados inteiramente na memória RAM, utilizando opcionalmente um arquivo binário (`minifs.dat`) como um "snapshot" para salvar e carregar seu estado entre as sessões.

### 2. Conceitos Fundamentais: A Arquitetura em Árvore
O MiniFS opera sob um paradigma análogo aos sistemas de arquivos UNIX, mas com simplificações estratégicas para fins didáticos. Sua arquitetura é construída sobre uma árvore N-ária, onde cada elemento, ou `Node`, pode representar um diretório ou um arquivo.
*   **Nós de Diretório (DIR_NODE):** São os "galhos" da árvore. Sua função principal é conter outros nós (seus "filhos"), formando a estrutura hierárquica. Um diretório pode conter uma mistura de outros diretórios e arquivos.
*   **Nós de Arquivo (FILE_NODE):** São as "folhas" da árvore. Eles não podem ter filhos e possuem um campo adicional para armazenar conteúdo textual (`char *content`).

A raiz de todo o sistema é um `Node` especial que representa o diretório `/`. Todas as operações (criar, mover, remover, listar) são, em sua essência, manipulações dessa árvore de nós em memória. Para facilitar a navegação, o sistema mantém um ponteiro global para o diretório de trabalho atual (`current_dir`), que serve como o ponto de referência para comandos com caminhos relativos e é modificado por comandos como `cd`.

### 3. Funcionalidades Principais: O Kit de Ferramentas do Usuário
O MiniFS oferece um conjunto de comandos essenciais, deliberadamente nomeados para serem familiares a qualquer usuário de um terminal UNIX, proporcionando uma transição suave do uso para o entendimento.
*   **Gerenciamento de Diretórios:** `mkdir` (cria um galho), `rm` (poda um galho, se ele não tiver outros galhos), `ls` (inspeciona o conteúdo de um galho), `cd` (muda seu ponto de observação na árvore), `pwd` (mostra onde você está na árvore a partir da raiz).
*   **Gerenciamento de Arquivos:** `touch` (cria uma folha vazia), `rm` (remove uma folha), `cat` (lê o conteúdo de uma folha), `echo` (escreve conteúdo em uma folha).
*   **Manipulação Estrutural:** `mv` (move/renomeia um nó, religando os ponteiros da árvore) e `cp` (realiza uma cópia profunda e recursiva de um nó e toda a sua subárvore).
*   **Ciclo de Vida e Persistência:** `exit` (salva o estado atual da árvore em disco antes de sair) e o carregamento automático na inicialização do programa.
*   **Visualização e Depuração:** `tree` (exporta a estrutura da árvore para um arquivo JSON, desacoplando a lógica em C da ferramenta de visualização).

### 4. Estrutura do Projeto: Um Design Modular e Limpo
O código é inteligentemente modularizado para promover o princípio da responsabilidade única, facilitando o entendimento, a manutenção e a extensão do projeto.
```bash
miniFS/
├── fs.c                # Implementa a API do sistema de arquivos. Toda a lógica de manipulação da árvore reside aqui.
├── fs.h                # Define as estruturas de dados (Node) e declara os protótipos da API do FS. É o "contrato" do sistema.
├── shell.c             # Implementa o loop do shell (REPL) e a lógica para interpretar comandos do usuário.
├── shell.h             # Declara o protótipo da função principal do shell, a única função que main.c precisa conhecer.
├── main.c              # Ponto de entrada. Orquestra o ciclo de vida: inicialização, execução do shell e finalização.
├── utils.c             # Contém funções utilitárias genéricas, como o processamento de strings, para manter outros arquivos limpos.
├── utils.h             # Declara os protótipos das funções utilitárias.
├── visualize.py        # Script Python desacoplado para renderizar a árvore de diretórios a partir de um arquivo JSON.
├── minifs.dat          # (Gerado) Arquivo binário que armazena o "snapshot" serializado do estado do sistema de arquivos.
└── fs_tree.json        # (Gerado) Arquivo JSON com a estrutura da árvore, servindo como interface para o visualizador.
```

### 5. Análise Detalhada do Código-Fonte: Mergulhando na Implementação
Esta seção disseca a anatomia do MiniFS, explicando o papel de cada componente e a lógica por trás de suas funções mais importantes.

#### A Estrutura de Dados `Node`: O DNA do Sistema
O `Node` é a molécula fundamental do MiniFS. Sua definição em `fs.h` revela a escolha de uma representação primeiro-filho/próximo-irmão (first-child/next-sibling), uma técnica clássica e eficiente para representar árvores N-árias com estruturas de dados binárias.
```c
typedef enum { FILE_NODE, DIR_NODE } NodeType;

typedef struct Node {
    char name[100];        // Nome do arquivo ou diretório.
    NodeType type;         // Tipo do nó (arquivo ou diretório).
    struct Node *parent;   // Ponteiro para o nó pai (navegação "para cima").
    struct Node *child;    // Ponteiro para o *primeiro* filho (se for diretório).
    struct Node *next;     // Ponteiro para o *próximo* irmão na lista de filhos do pai.
    char *content;         // Conteúdo, se for um arquivo (alocado dinamicamente).
} Node;
```
*   `parent`: Essencial para operações como `cd ..` e para a função `pwd`, que precisa reconstruir o caminho completo subindo na hierarquia até a raiz.
*   `child`: Em um nó de diretório, aponta para o início de uma lista encadeada de seus filhos. Em um arquivo, é sempre `NULL`.
*   `next`: Este ponteiro é o que forma a lista encadeada de irmãos. Se um diretório D contém os arquivos F1, F2, e F3, a estrutura de ponteiros será: `D->child` aponta para F1. `F1->next` aponta para F2. `F2->next` aponta para F3. `F3->next` é `NULL`. Essa abordagem é mais flexível e eficiente em memória do que usar um array de ponteiros para filhos, pois não exige alocação contígua nem pré-definição de um número máximo de filhos.

#### `fs.c` & `fs.h`: O Coração Lógico do Sistema
Estes arquivos contêm a "mágica" do sistema de arquivos. `fs.h` é o contrato público (a API), e `fs.c` é a implementação privada.

*   **Funções de Resolução de Caminho (Path Resolution):**
    *   `find_node_in_dir(dir, name)`: A busca mais fundamental. Itera através da lista encadeada de filhos de `dir` (começando em `dir->child` e seguindo os ponteiros `next`) até encontrar um nó com o nome correspondente.
    *   `find_node_by_path(path)`: O "GPS" do sistema. Esta função é a mais crítica para a navegação. Ela recebe um caminho (ex: `/home/user` ou `docs/report.txt`), o "tokeniza" usando `/` como delimitador, e desce na árvore a partir de um ponto de partida (a raiz para caminhos absolutos, `current_dir` para relativos). Trata os casos especiais `.` (não faz nada, continua no mesmo diretório) e `..` (navega para cima usando o ponteiro `parent`).
    *   `get_parent_dir_and_basename(path, out_basename)`: Uma função auxiliar crucial que encapsula uma lógica complexa. Dada uma entrada como `/a/b/c`, ela precisa retornar um ponteiro para o nó do diretório pai (`/a/b`) e extrair o nome do nó final (`c`). Ela usa as funções `dirname()` e `basename()` (da `libgen.h`), que são padrões POSIX, para realizar essa separação. Isso simplifica imensamente comandos como `mkdir` e `touch`, que agora só precisam chamar esta função para saber onde criar e com que nome.

*   **Funções de Manipulação da Árvore:**
    *   `attach_node(parent, child)`: "Enxerta" um novo nó (`child`) na lista de filhos de um `parent`. Ela navega até o final da lista de irmãos (percorrendo os ponteiros `next` a partir de `parent->child`) e anexa o novo nó, atualizando os ponteiros `next` e `parent` corretamente.
    *   `detach_node(node)`: "Poda" um nó da árvore. Esta operação requer cuidado: se o nó a ser removido é o primeiro filho (`parent->child == node`), o ponteiro `parent->child` deve ser atualizado para `node->next`. Caso contrário, é preciso encontrar o irmão anterior ao nó para que seu ponteiro `next` possa "pular" o nó removido e apontar para `node->next`, mantendo a integridade da lista encadeada.
    *   `fs_mkdir(path)` e `fs_touch(path)`: Usam `get_parent_dir_and_basename` para encontrar o diretório pai e o nome do novo nó. Verificam se o nome já existe no diretório pai usando `find_node_in_dir` para evitar duplicatas. Alocam um novo `Node` com `malloc`, inicializam seus campos e, por fim, o anexam à árvore com `attach_node`.
    *   `fs_rm(path)`: Localiza o nó com `find_node_by_path`. Realiza verificações de segurança cruciais: não permite remover a raiz (`/`) e nem diretórios que não estejam vazios (`target->child != NULL`). Se as verificações passarem, ele chama `detach_node` para desconectá-lo da árvore e depois chama `fs_destroy` (uma função recursiva de limpeza) para liberar a memória do nó removido e de seu conteúdo.

*   **Comandos de Movimentação e Cópia:**
    *   `fs_mv(source_path, dest_path)`: Esta é uma operação primariamente lógica e, portanto, muito rápida. A "mágica" do `mv` é que ele não move dados, apenas reconfigura ponteiros. Ele localiza o nó de origem e o diretório de destino, chama `detach_node` na origem e `attach_node` no destino. Se o destino for um novo nome de arquivo, ele também atualiza `source_node->name`. É o equivalente a mudar um funcionário de departamento em um organograma.
    *   `fs_cp(source_path, dest_path)`: Em contraste com `mv`, `cp` é uma operação "física" e computacionalmente mais cara. Ela envolve uma cópia profunda e recursiva. A função `copy_node_recursive` é chamada para criar uma duplicata exata do nó de origem. Se for um arquivo, seu `content` é duplicado com `strdup` (que aloca nova memória e copia a string). Se for um diretório, a função se chama recursivamente para todos os seus filhos, recriando toda a subárvore. A nova árvore copiada é então anexada ao destino com `attach_node`.

*   **Serialização (Persistência):**
    *   `fs_save(filepath)`: Abre um arquivo em modo de escrita binária (`wb`) e inicia o processo de serialização com `save_node_recursive(file, root)`.
    *   `save_node_recursive`: Percorre a árvore em pré-ordem (Pre-order traversal: Pai -> Filhos). Essa ordem é crucial porque garante que, ao carregar, o nó pai já existirá quando seus filhos forem lidos. Para cada nó, ela escreve no arquivo um registro bem definido: o `NodeType`, o comprimento do nome, a string do nome, e (se for um arquivo) o comprimento do conteúdo e a string do conteúdo. Finalmente, escreve o número de filhos e se chama recursivamente para cada um.
    *   `fs_load(filepath)` e `load_node_recursive`: Fazem o processo inverso. Elas leem os dados do arquivo na mesma ordem em que foram escritos, usam `malloc` para reconstruir a árvore nó por nó na memória, e religam os ponteiros `parent`, `child` e `next` para restaurar a estrutura idêntica à que foi salva.

#### `shell.c` & `shell.h`: A Interface com o Usuário
Este módulo é o front-end do sistema, responsável por toda a interação com o usuário final.
*   **shell_loop():** O coração do shell. É um loop `while(running)` que implementa o ciclo clássico REPL (Read-Eval-Print Loop).
    *   **Print:** Chama `print_prompt()` para exibir o prompt dinâmico (ex: `MiniFS:/home/user$`).
    *   **Read:** Usa `fgets` para ler a linha de comando inserida pelo usuário de forma segura (evitando buffer overflows).
    *   **Eval:**
        *   Usa `split_string` (de `utils.c`) para quebrar a entrada em um array de tokens (`argv`), simulando o comportamento de um shell real.
        *   Usa uma cadeia de `if-else if` para comparar o primeiro token (`argv[0]`) com os nomes dos comandos conhecidos ("mkdir", "ls", "cd", etc.).
        *   Com base no comando, invoca a função apropriada da API do `fs.c`, passando os argumentos necessários (`argv[1]`, `argv[2]`).
        *   Realiza a validação básica do número de argumentos antes de chamar a API, fornecendo feedback útil ao usuário.
*   **print_prompt():** Constrói a string do prompt dinamicamente. Começando do `current_dir`, ele navega para cima na árvore usando os ponteiros `parent` até chegar à raiz, concatenando os nomes dos nós no caminho para formar o caminho absoluto.

#### `main.c`: O Ciclo de Vida da Aplicação
Este é o ponto de entrada (`main`) do programa. Sua responsabilidade é gerenciar o ciclo de vida completo da aplicação de forma ordenada.
*   **Inicialização (Startup):** Chama `fs_load(SAVE_FILE)`. Esta função tenta carregar o estado anterior do sistema de arquivos a partir de `minifs.dat`. Se o arquivo não existir (primeira execução), `fs_load` inteligentemente chama `fs_init` para criar um sistema de arquivos novo e vazio, com apenas o diretório raiz (`/`).
*   **Execução (Runtime):** Inicia o `shell_loop()`, transferindo o controle do programa para o usuário. O `main` fica em espera até que o loop do shell termine.
*   **Finalização (Shutdown):** Quando o `shell_loop` termina (após o usuário digitar `exit`), o `main` retoma o controle e executa duas tarefas cruciais de limpeza:
    *   `fs_save(SAVE_FILE)`: Salva o estado atual da árvore no disco, garantindo a persistência.
    *   `fs_destroy(root)`: Percorre toda a árvore em pós-ordem e libera toda a memória alocada dinamicamente com `malloc` e `strdup`, prevenindo vazamentos de memória (memory leaks), uma prática fundamental em C.

#### `utils.c` & `utils.h`: Funções de Apoio Essenciais
Este módulo abstrai funcionalidades genéricas para manter o resto do código focado em sua lógica principal.
*   `split_string(input, count)`: Uma robusta função de parsing de string. Recebe uma linha de entrada, remove espaços em branco no início e no fim (`trim_whitespace`), e usa `strtok` (uma função padrão de C para tokenização) para dividi-la em palavras. Retorna um array de strings (`char**`) alocado dinamicamente, que o `shell.c` pode usar como `argv`.

#### `visualize.py`: Tornando o Invisível, Visível
Este script é uma ferramenta auxiliar externa, um excelente exemplo de desacoplamento. Ele não interage diretamente com o programa em C.
*   **Funcionamento:** Sua única tarefa é ler o arquivo `fs_tree.json`, que é gerado pelo comando `tree` no shell do MiniFS. O comando `tree` invoca `fs_export_tree_json`, que percorre a árvore em C e escreve uma representação textual da hierarquia em formato JSON, um padrão universal de troca de dados. O script Python, então, parseia este JSON e usa uma função recursiva (`print_tree`) para imprimir a árvore no terminal, utilizando a biblioteca `colorama` para adicionar cores e ícones (pastas e arquivos), tornando a estrutura de ponteiros em memória, que é inerentemente abstrata, em algo concreto e fácil de entender.

### 6. Como Compilar e Usar: Do Código-Fonte ao Shell Interativo
#### Pré-requisitos
*   Um compilador C moderno (ex: `gcc` ou `clang`).
*   Python 3 para executar o script de visualização.
*   A biblioteca Python `colorama`: `pip install colorama`.

#### Compilação Detalhada
Para compilar, navegue até o diretório raiz do projeto e execute o comando:
```bash
gcc -o minifs main.c fs.c shell.c utils.c -I. -std=c99 -Wall
```
*   `gcc`: O compilador C do GNU.
*   `-o minifs`: Especifica que o nome do arquivo executável de saída será `minifs`.
*   `main.c fs.c shell.c utils.c`: A lista de todos os arquivos de código-fonte que devem ser compilados e ligados (linked) juntos para formar o programa final.
*   `-I.`: Informa ao pré-processador para procurar arquivos de cabeçalho (`.h`) no diretório atual (`.`), o que é necessário para que `#include "fs.h"` funcione corretamente.
*   `-std=c99`: Assegura que o código seja compilado de acordo com o padrão C99, que inclui características usadas no projeto.
*   `-Wall`: (Warning all) Ativa todos os avisos do compilador. Esta é uma prática recomendada para escrever código C robusto, pois ajuda a identificar problemas potenciais que não são erros de sintaxe, como variáveis não utilizadas ou conversões de tipo arriscadas.

#### Execução
Após a compilação, um arquivo executável `minifs` será criado. Inicie o shell com:
```bash
./minifs
```
Na primeira vez, ele criará um sistema de arquivos vazio. Nas execuções subsequentes, ele carregará o estado salvo em `minifs.dat`.
Contudo, uma árvore de teste pode ser carregada a partir do código de `setup.txt`, um arquivo que pode ser executado juntamente ao `./minifs` a fim de criar uma árvore inteira como exemplo para estudos. Mais detalhes sobre o uso serão descritos abaixo!

#### Guia de Comandos Completo

| Comando | Sintaxe | Descrição Detalhada |
| :--- | :--- | :--- |
| `mkdir` | `mkdir <caminho_dir>` | Cria um novo diretório no caminho especificado. Pode ser um caminho absoluto (ex: `/home/user`) ou relativo (ex: `docs`). |
| `touch` | `touch <caminho_arq>` | Cria um novo arquivo vazio. Se o arquivo já existir, não faz nada (semelhante ao comportamento UNIX). |
| `ls` | `ls [caminho]` | Lista o conteúdo do diretório. Se o caminho for omitido, lista o diretório atual. Se o caminho for o de um arquivo, simplesmente printa seu nome (já que não é um diretório) |
| `cd` | `cd <caminho_dir>` | Altera o diretório de trabalho atual. Suporta `.` (diretório atual) e `..` (diretório pai). |
| `pwd` | `pwd` | Exibe o caminho completo (absoluto) do diretório de trabalho atual, da raiz até o nó atual. |
| `rm` | `rm <caminho>` | Remove um arquivo ou um diretório vazio. Impede a remoção de diretórios não vazios ou do diretório raiz `/` para segurança. |
| `cat` | `cat <caminho_arq>` | Exibe o conteúdo de um arquivo de texto no terminal. |
| `echo` | `echo <conteudo> > <caminho_arq>` | Escreve ou sobrescreve o conteúdo de um arquivo. O conteúdo pode conter espaços, mas não reconhece algarismos especiais (como 'ç' ou vogais acentuadas). |
| `mv` | `mv <origem> <destino>` | Move ou renomeia um arquivo ou diretório. É uma operação de re-ponteiramento, muito eficiente. |
| `cp` | `cp <origem> <destino>` | Copia um arquivo ou diretório. Para diretórios, a cópia é recursiva, criando uma duplicata completa da subárvore. |
| `tree` | `tree` | Exporta a estrutura atual do sistema de arquivos para `fs_tree.json` e notifica o usuário para usar `visualize.py`. |
| `exit` | `exit` | Salva o estado atual do sistema em `minifs.dat` e encerra o programa de forma limpa. |

### 7. Guia Prático: Uma Sessão de Teste Completa
Esta seção é um tutorial passo a passo que demonstra um ciclo de uso completo do MiniFS: compilação, criação de uma estrutura de diretórios e arquivos, manipulação desses itens, salvamento do estado e, finalmente, a visualização gráfica da árvore resultante.

#### Passo 1: Compilação e Primeira Execução
Primeiro, certifique-se de ter o compilador gcc baixado (ou qualquer outro que saibas usar) e estar no diretório raiz do projeto, onde os arquivos `.c` estão localizados. Compile o programa usando o comando que já detalhamos:
```bash
# Este comando é executado no seu terminal (Bash, Zsh, etc.)
gcc -o minifs main.c fs.c shell.c utils.c -I. -std=c99 -Wall
```
Se tudo ocorrer bem, um executável chamado `minifs` será criado. Agora, vamos executá-lo pela primeira vez:
```bash
# Execute o programa recém-compilado
./minifs
```
Como o arquivo `minifs.dat` ainda não existe, o programa exibirá uma mensagem informando que não encontrou o arquivo e criará um novo sistema de arquivos vazio. Você será recebido pelo prompt do MiniFS, posicionado no diretório raiz (`/`):
```shell
File 'minifs.dat' not found. Initializing a new filesystem.
MiniFS:/$
```

#### Passo 2: Construindo a Estrutura de Arquivos
Vamos criar uma hierarquia de diretórios e arquivos simples, semelhante a um diretório pessoal.
Crie os diretórios `home` e `user`:
```shell
MiniFS:/$ mkdir home
MiniFS:/$ cd home
MiniFS:/home$ mkdir user
MiniFS:/home$ cd user
MiniFS:/home/user$
```
Observe como o prompt é atualizado dinamicamente para refletir o diretório de trabalho atual.
Crie subdiretórios e um arquivo dentro de `/home/user`:
```shell
MiniFS:/home/user$ mkdir docs
MiniFS:/home/user$ mkdir projects
MiniFS:/home/user$ touch readme.txt
```
Liste o conteúdo para verificar:
```shell
MiniFS:/home/user$ ls
docs/  projects/  readme.txt
```
O comando `ls` mostra os diretórios e o arquivo que acabamos de criar.

#### Passo 3: Editando Conteúdo e Movendo Arquivos
Agora, vamos manipular os arquivos que criamos.
Adicione conteúdo ao `readme.txt` usando `echo` e verifique com `cat`:
```shell
MiniFS:/home/user$ echo Este eh um projeto de teste para o MiniFS. > readme.txt
MiniFS:/home/user$ cat readme.txt
Este eh um projeto de teste para o MiniFS.
```
Crie um arquivo dentro de `docs` e depois o renomeie usando `mv`:
```shell
MiniFS:/home/user$ touch docs/report.docx
MiniFS:/home/user$ mv docs/report.docx docs/report.txt
```
Mova o arquivo `readme.txt` para o diretório `docs`:
```shell
MiniFS:/home/user$ mv readme.txt docs/
```
Verifique o resultado final com `ls`:
```shell
MiniFS:/home/user$ ls
docs/  projects/

MiniFS:/home/user$ ls docs
readme.txt  report.txt
```
Como podemos ver, `readme.txt` não está mais em `/home/user`, mas agora reside dentro de `docs` junto com `report.txt`.

#### Passo 4: Salvando o Estado e Verificando a Persistência
Esta é uma das funcionalidades mais importantes do MiniFS. Vamos salvar nosso trabalho.
Saia do shell com o comando `exit`:
```shell
MiniFS:/home/user$ exit
Saving filesystem to minifs.dat... Done.
Exiting.
```
Neste momento, toda a árvore de nós que criamos na memória, incluindo o conteúdo dos arquivos e nosso diretório atual, foi serializada e salva no arquivo `minifs.dat`.
Reinicie o MiniFS para testar a persistência:
```bash
# De volta ao seu terminal do sistema
./minifs
```
Observe o prompt e verifique o estado:
Ao iniciar, o MiniFS carregará o estado a partir de `minifs.dat`. Note que o prompt já o coloca de volta exatamente onde você parou:
```shell
MiniFS:/home/user$
```
Isso prova que o `current_dir` também foi salvo e restaurado.
Vamos confirmar que tudo está intacto:
```shell
MiniFS:/home/user$ pwd
/home/user
MiniFS:/home/user$ ls
docs/  projects/
MiniFS:/home/user$ ls docs
readme.txt  report.txt
```
Sucesso! O sistema de arquivos foi restaurado perfeitamente.

#### Passo 5: Visualizando a Árvore de Arquivos
Finalmente, vamos usar a ferramenta de visualização em Python.
Gere o arquivo JSON com o comando `tree`:
Dentro do shell do MiniFS, execute o comando `tree`.
```shell
MiniFS:/home/user$ tree
Tree structure exported to fs_tree.json. Use visualize.py to view.
```
Saia do MiniFS para executar o script Python:
Você pode sair com `exit` (que salvará o estado novamente, o que não tem problema).
```shell
MiniFS:/home/user$ exit
```
Execute o script `visualize.py` no seu terminal:
Certifique-se de ter a biblioteca `colorama` instalada (`pip install colorama`).
```bash
# No seu terminal do sistema
python3 visualize.py
```
Veja a representação gráfica da sua estrutura:
O script lerá `fs_tree.json` e imprimirá uma bela representação colorida da sua árvore de arquivos no terminal, que se parecerá com isto:
```
📁 /
└── 📁 home
    └── 📁 user
        ├── 📁 docs
        │   ├── 📄 readme.txt
        │   └── 📄 report.txt
        └── 📁 projects
```
Este guia prático demonstrou o fluxo de trabalho completo do MiniFS, ilustrando como os conceitos teóricos de manipulação da árvore, persistência e visualização se unem para criar uma experiência de sistema de arquivos funcional e compreensível.

Se você quiser uma visualização mais rápida e estruturada da criação de uma árvore, podes utilizar o modelo de árvore criado pelo arquivo `setup.txt` através do seguinte comando no terminal (antes de abrir o ./minifs):
```shell
Get-Content setup.txt | .\miniFS.exe
```
Com esse comando, toda a estrutura de `setup.txt` será montada, já havendo sido utilizados os comandos `tree` e `exit` nela, bastando a você apenas executar o comando com python para visualizar a árvore (ou executar o miniFS e verificar você mesmo os diretórios e arquivos com os comandos ensinados nesta seção). Teste você mesmo e veja a estrutura formada! :)

### 8. Comparação Aprofundada com FAT (MS-DOS): Abstração vs. Realidade Física
Para apreciar plenamente o design do MiniFS, é instrutivo compará-lo a um sistema de arquivos real como o FAT (File Allocation Table), que dominou a era do MS-DOS e ainda é usado hoje em pen drives e cartões SD.

#### Tabela Comparativa Expandida

| Critério | MiniFS | DOS (FAT) |
| :--- | :--- | :--- |
| **Tipo de Sistema** | Virtual (simulado em memória RAM) | Real (gerencia armazenamento físico) |
| **Estrutura de Dados Primária** | Árvore de nós em RAM, conectada por ponteiros C. | Tabela de Alocação de Arquivos (um array em disco) + Entradas de Diretório. |
| **Mapeamento de Conteúdo** | Um ponteiro `char *content` aponta para um bloco único de memória na heap. | Conteúdo é dividido em clusters (blocos de disco). A FAT forma uma lista encadeada de clusters. |
| **Persistência** | Manual, via serialização para um único arquivo (`minifs.dat`). | Automática e contínua, diretamente nos setores do disco. |
| **Fragmentação** | Não há fragmentação de arquivos no sentido de disco. A fragmentação pode ocorrer na heap do processo, mas é gerenciada pelo SO. | A fragmentação de arquivos é um problema central. Os clusters de um arquivo podem estar espalhados por todo o disco. |
| **Alocação de Espaço** | Dinâmica via `malloc()` na RAM. O limite é a memória disponível para o processo. | Gerencia uma lista de clusters livres e ocupados no disco. O limite é o tamanho da partição. |
| **Metadados** | Apenas nome e tipo. | Nome, atributos (leitura, oculto, etc.), data e hora da última modificação, e o ponteiro para o primeiro cluster. |
| **Tolerância a Falhas** | Nenhuma. Se `minifs.dat` corromper, o sistema é perdido. | Limitada. FAT32 mantém uma cópia de backup da FAT. Ferramentas como `CHKDSK` podem tentar reparar inconsistências. |
| **Controle de Permissões** | Inexistente. | Inexistente. É um sistema de usuário único sem conceito de propriedade ou permissões. |
| **Visualização Externa** | Exportação via JSON + Script Python. | Ferramentas do sistema operacional (`dir`, `tree`, exploradores de arquivos). |

#### Diferenças Conceituais e Técnicas Cruciais
A distinção mais profunda está em como a estrutura e o conteúdo são mapeados.
*   **Mapeamento em FAT:** Imagine um livro de registro (as entradas de diretório) e um índice gigante (a tabela FAT). Para ler um arquivo, você primeiro encontra sua entrada no livro de registro, que lhe diz a página inicial (o primeiro cluster). Você vai até essa página. No final da página, em vez de um número de página seguinte, você volta ao índice, olha na entrada correspondente à página que acabou de ler, e ele lhe diz qual é a próxima página a ser lida. Você repete isso até que o índice diga "Fim do Arquivo". Essa é a essência da lista encadeada de clusters da FAT. A estrutura de diretórios em si também é armazenada dessa maneira.
*   **Mapeamento em MiniFS:** Imagine um organograma de uma empresa. A relação entre um gerente (diretório) e seus subordinados diretos (arquivos/subdiretórios) é explícita. O nó do gerente tem um ponteiro `child` que aponta diretamente para o primeiro subordinado. Esse subordinado tem um ponteiro `next` que aponta para seu colega ao lado, e assim por diante. Não há uma tabela de índice externa; a estrutura está contida nos próprios nós. O conteúdo de um arquivo é simplesmente um ponteiro para um único documento de texto na memória.

*   **Abstração vs. Realidade:**
    *   FAT é uma abstração de baixo nível, projetada para a dura realidade do hardware de disco. Sua complexidade (gerenciamento de clusters, tratamento de setores defeituosos, limites de tamanho de nome de arquivo) é uma resposta direta às limitações físicas dos dispositivos de armazenamento.
    *   MiniFS é uma abstração de alto nível, focada exclusivamente na estrutura lógica e hierárquica de um sistema de arquivos. Ao ignorar as complexidades do hardware, ele pode apresentar os conceitos de árvore, navegação e manipulação de nós de forma pura, clara e direta.

### 9. Vantagens e Proposta Educacional: Mais que um Projeto, um Laboratório
O MiniFS não compete com sistemas reais; ele brilha como uma ferramenta de aprendizado.
*   **Didático e Simples: Um Laboratório de Sistemas de Arquivos:** A maior vantagem é a clareza. A separação entre a lógica (`fs.c`) e a interface (`shell.c`) permite que um estudante se concentre em entender a manipulação da árvore sem se distrair com o parsing de entrada. O código é procedural e usa estruturas de dados fundamentais, tornando-o acessível e, ao mesmo tempo, um ótimo exemplo de engenharia de software em C.
*   **Ambiente Controlado e Seguro:** Permite a experimentação sem medo. Um `rm -rf /` acidental no MiniFS não tem consequências no sistema operacional hospedeiro. Isso incentiva a exploração e o teste de todos os comandos, incluindo os destrutivos, que é uma parte crucial do aprendizado prático.
*   **Visualização Concreta (Tornando o Invisível, Visível):** Estruturas de dados em árvore e listas encadeadas de ponteiros podem ser difíceis de visualizar mentalmente. O comando `tree` e o script `visualize.py` transformam a estrutura de ponteiros na memória em uma representação gráfica e intuitiva. Isso cria uma ponte essencial entre o código, a estrutura de dados e o resultado conceitual, solidificando o entendimento.
*   **Código Aberto e Extensível: Um Ponto de Partida:** O projeto serve como um "esqueleto" robusto. Um estudante pode facilmente estendê-lo, transformando-o de um projeto de demonstração para um projeto de portfólio. Exercícios de extensão possíveis incluem:
    *   Implementar um sistema de permissões (leitura/escrita).
    *   Adicionar suporte para links simbólicos ou hard links.
    *   Incluir timestamps (data de criação/modificação) nos nós.
    *   Implementar um sistema de cotas de espaço por diretório.
    *   Consertar alguns erros de eficiência de espaço ou desempenho que escaparam à atenção dos desenvolvedores durante a rápida e corrida construção. Nós sabemos de alguns, mas vocês serão capazes de os achar e consertá-los? ;)

### 10. Limitações do Projeto: Escolhas de Design Deliberadas
As limitações do MiniFS não são falhas, mas sim escolhas de design intencionais para manter o foco pedagógico e a simplicidade.
*   **Não Gerencia Disco Real:** Esta é a principal simplificação. Implementar o gerenciamento de blocos em disco introduziria uma camada de complexidade (drivers, acesso a setores, bitmaps de alocação) que desviaria o foco do objetivo principal de simular um sistema de arquivos em árvore e permanência externa sequencial.
*   **Sem Controle de Espaço:** O sistema depende do `malloc` da biblioteca padrão. Se a RAM acabar, `malloc` retornará `NULL` e o programa falhará. Implementar cotas ou verificação de espaço exigiria uma camada extra de gerenciamento que complicaria o núcleo do código.
*   **Sem Permissões:** Um sistema de permissões UNIX-like exigiria a adição de campos de proprietário, grupo e modo a cada `Node`, além de estruturas para usuários/grupos e verificações de acesso em quase todas as funções da API, aumentando significativamente a complexidade. Ao omiti-lo, o foco permanece na estrutura.
*   **Sem Metadados Avançados:** Timestamps, links, ou atributos estendidos adicionariam mais campos à `struct Node` e uma lógica mais complexa às funções de manipulação (`cp`, `rm`), tornando o núcleo do sistema mais difícil de entender para um iniciante.
*   **Não é Concorrente (Single-Threaded):** O sistema usa variáveis globais (`root`, `current_dir`) e não possui nenhum mecanismo de bloqueio (mutexes, semáforos). Se dois threads tentassem modificar a árvore simultaneamente (ex: `mv /a/b /c` e `rm /a`), a estrutura de dados seria corrompida. Manter o projeto single-threaded é uma escolha fundamental para preservar a simplicidade e focar nos algoritmos estruturais.

### 11. Conclusão: Uma Ponte entre a Teoria e a Prática
O MiniFS é uma ferramenta educacional exemplar que simula com sucesso e elegância o funcionamento de um sistema de arquivos em um ambiente controlado e compreensível. Sua comparação com o FAT lança luz sobre seus objetivos distintos: enquanto o FAT é uma solução de engenharia para problemas do mundo real em hardware físico, o MiniFS é uma solução pedagógica, otimizada para o ensino, a clareza e a extensibilidade.
Com sua interface de linha de comando familiar, visualização gráfica inovadora e código-fonte limpo e dissecado, o MiniFS serve como uma plataforma ideal e uma ponte robusta entre a teoria abstrata das estruturas de dados e a implementação prática dos sistemas operacionais. Ele é um ponto de partida excepcional para qualquer estudante ou desenvolvedor que deseje aprofundar seu entendimento sobre como os sistemas de arquivos realmente funcionam "por baixo dos panos".
