# miniFS/visualize.py

import json
import os
import sys

try:
    from colorama import init, Fore, Style
except ImportError:
    print("Módulo 'colorama' não encontrado. Por favor, instale-o com: pip install colorama")
    sys.exit(1)

# Inicializa o colorama (necessário para Windows)
init(autoreset=True)

# Ícones e cores
DIR_ICON = "📁"
FILE_ICON = "📄"
DIR_COLOR = Fore.YELLOW
FILE_COLOR = Fore.CYAN
STRUCTURE_COLOR = Fore.WHITE + Style.DIM
ERROR_COLOR = Fore.RED
INFO_COLOR = Fore.YELLOW

def print_tree(node, prefix="", is_last=True):
    """
    Função recursiva para imprimir a árvore de diretórios.
    """
    # Conector da árvore
    connector = "└── " if is_last else "├── "
    
    # Define o ícone e a cor com base no tipo do nó
    if node.get('type') == 'directory':
        icon = DIR_ICON
        color = DIR_COLOR
    else:
        icon = FILE_ICON
        color = FILE_COLOR

    # Imprime a linha atual
    print(f"{prefix}{STRUCTURE_COLOR}{connector}{Style.RESET_ALL}{color}{icon} {node.get('name', 'Unnamed')}{Style.RESET_ALL}")

    # Prepara o prefixo para os nós filhos
    child_prefix = prefix + ("    " if is_last else "│   ")

    # Se o nó é um diretório e tem filhos, imprime-os recursivamente
    children = node.get('children', [])
    if children:
        for i, child in enumerate(children):
            is_child_last = (i == len(children) - 1)
            print_tree(child, prefix=child_prefix, is_last=is_child_last)

def main():
    """
    Função principal: carrega o JSON e inicia a impressão da árvore.
    """
    json_path = 'fs_tree.json'
    
    if not os.path.exists(json_path):
        print(f"{ERROR_COLOR}Erro: Arquivo '{json_path}' não encontrado.")
        print(f"{INFO_COLOR}Execute o comando 'tree' dentro do shell MiniFS para gerá-lo.")
        return

    try:
        with open(json_path, 'r', encoding='utf-8') as f:
            # Verifica se o arquivo não está vazio
            content = f.read()
            if not content:
                print(f"{ERROR_COLOR}Erro: O arquivo '{json_path}' está vazio.")
                print(f"{INFO_COLOR}Tente executar o comando 'tree' novamente no MiniFS.")
                return
            root_node = json.loads(content)
    except json.JSONDecodeError:
        print(f"{ERROR_COLOR}Erro: Falha ao decodificar o arquivo JSON. Pode estar mal formatado.")
        return
    except Exception as e:
        print(f"{ERROR_COLOR}Ocorreu um erro inesperado: {e}")
        return

    # Inicia a impressão a partir do nó raiz
    print_tree(root_node)

if __name__ == "__main__":
    main()