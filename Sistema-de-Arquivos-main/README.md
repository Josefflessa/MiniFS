# 🗂️ MiniFS - Um Sistema de Arquivos Virtual em C

MiniFS é um simulador de sistema de arquivos hierárquico, implementado inteiramente em C. Ele opera em memória e simula os comandos de um shell Unix, como `mkdir`, `cd`, `ls`, `mv`, e `cp`, com suporte a caminhos relativos e absolutos.

Este projeto é um exercício educacional focado em conceitos de Sistemas Operacionais, manipulação de memória e estruturas de dados em C (árvores n-árias).

## 🎯 Funcionalidades

- **Hierarquia de Diretórios**: Navegue por uma árvore de diretórios como em sistemas reais, usando caminhos como `/docs/reports` e `../images`.
- **Manipulação de Arquivos e Diretórios**:
    - `mkdir <path>`: Cria um novo diretório no caminho especificado.
    - `touch <path>`: Cria um novo arquivo vazio.
    - `ls [path]`: Lista o conteúdo do diretório atual ou de um caminho especificado.
    - `cd <path>`: Entra em um diretório. `cd ..` para voltar, `cd /` para ir à raiz.
    - `rm <path>`: Remove um arquivo ou diretório (deve estar vazio).
    - `pwd`: Mostra o caminho absoluto atual.
    - `mv <source> <dest>`: Move ou renomeia um arquivo/diretório.
    - `cp <source> <dest>`: Copia um arquivo ou diretório recursivamente.
- **Manipulação de Conteúdo**:
    - `cat <path>`: Exibe o conteúdo de um arquivo.
    - `echo <texto> > <path>`: Escreve (ou sobrescreve) conteúdo em um arquivo.
- **Persistência**:
    - O estado do sistema de arquivos é salvo automaticamente no arquivo `minifs.dat` ao sair com `exit`.
    - Ao iniciar, o programa carrega automaticamente o estado do `minifs.dat`, se ele existir.
- **Visualização da Árvore**:
    - `tree`: Exporta a estrutura atual do sistema de arquivos para um arquivo `fs_tree.json`.
    - `python visualize.py`: Um script Python que lê o arquivo JSON e exibe uma árvore visualmente agradável no terminal.

## 🛠️ Como Compilar e Executar

### Pré-requisitos
- Um compilador C (como `gcc` ou `clang`).
- `make`.
- `python3` e o pacote `colorama` (`pip install colorama`).

### Passos

1.  **Clone o repositório ou salve todos os arquivos.**

2.  **Compile o projeto:**
    ```sh
    make
    ```
    Isso irá gerar um executável chamado `minifs`.

3.  **Execute o programa:**
    ```sh
    ./minifs
    ```

4.  **Para visualizar a árvore (após usar o comando `tree` no MiniFS):**
    ```sh
    python visualize.py
    ```

5.  **Limpe os arquivos compilados (e os arquivos de save/tree):**
    ```sh
    make clean
    ```

## 🚀 Exemplo de Uso

```sh
$ make
$ ./minifs
No save file found. Starting a new file system.
MiniFS:/$ mkdir /docs
MiniFS:/$ mkdir /docs/reports
MiniFS:/$ touch /docs/reports/final.txt
MiniFS:/$ echo "Final Report Content" > /docs/reports/final.txt
MiniFS:/$ cp /docs/reports /docs/reports_backup
MiniFS:/$ ls /docs
d reports/
d reports_backup/
MiniFS:/$ tree
File system tree exported to fs_tree.json
MiniFS:/$ exit
File system saved to minifs.dat
Exiting MiniFS. Goodbye!

# Agora, no terminal do seu sistema operacional:
$ python visualize.py
└── 📁 /
    └── 📁 docs
        ├── 📁 reports
        │   └── 📄 final.txt
        └── 📁 reports_backup
            └── 📄 final.txt
```

## 📐 Estrutura de Dados

O sistema é baseado em uma árvore n-ária, onde cada nó (`struct Node`) representa um arquivo ou um diretório. A relação pai-filho-irmão é gerenciada por ponteiros:
- `parent`: Aponta para o diretório pai.
- `child`: Aponta para o primeiro item dentro de um diretório.
- `next`: Aponta para o próximo item no mesmo nível (irmão).

```c
typedef struct Node {
    char name[100];
    NodeType type;
    struct Node *parent;
    struct Node *child;
    struct Node *next;
    char *content;
} Node;
```
