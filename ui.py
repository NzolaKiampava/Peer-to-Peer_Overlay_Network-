"""
Interface de usuário e comandos - Versão melhorada com cores
"""

import os
import sys
from peer_client import peer_client_register, peer_client_unregister, peer_client_get_peers
from protocol import (
    establish_connections, remove_neighbor, count_external_neighbors,
    find_neighbor_by_seqnumber, protocol_query_identifier, tcp_receive
)
from network import close_socket


# Cores ANSI para terminal
class Colors:
    """Cores para terminal"""
    HEADER = '\033[95m'
    BLUE = '\033[94m'
    CYAN = '\033[96m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    RED = '\033[91m'
    GRAY = '\033[90m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'
    END = '\033[0m'


def clear_screen():
    """Limpa a tela"""
    os.system('cls' if os.name == 'nt' else 'clear')


def print_header():
    """Imprime cabeçalho"""
    print(f"\n{Colors.BOLD}{Colors.CYAN}")
    print("╔═══════════════════════════════════════════════════════════╗")
    print("║                                                           ║")
    print("║          P2P Overlay Network - Python Edition            ║")
    print("║                                                           ║")
    print("╚═══════════════════════════════════════════════════════════╝")
    print(f"{Colors.END}")


def print_status(state):
    """Imprime status atual do peer"""
    status = f"{Colors.GREEN}✓ ONLINE{Colors.END}" if state.joined else f"{Colors.RED}✗ OFFLINE{Colors.END}"
    seqnum = f"#{state.seqnumber}" if state.seqnumber else "-"
    neighbors = len(state.neighbors.neighbors) if state.joined else 0
    
    print(f"\n{Colors.BOLD}Status:{Colors.END}")
    print(f"  Conexão: {status}")
    print(f"  SeqNumber: {seqnum}")
    print(f"  Vizinhos: {neighbors}")
    print(f"  Servidor: {state.server_ip}:{state.server_port}")
    print()


def print_help():
    """Imprime ajuda com formatação melhorada"""
    print(f"\n{Colors.BOLD}{Colors.YELLOW}=== COMANDOS DISPONÍVEIS ==={Colors.END}\n")
    
    commands = [
        ("join", "Entrar na rede overlay"),
        ("leave", "Sair da rede"),
        ("show neighbors", "Listar todos os vizinhos (internos e externos)"),
        ("release <id>", "Desconectar de um vizinho interno"),
        ("list identifiers", "Listar identificadores locais"),
        ("post <id>", "Publicar um novo identificador"),
        ("search <id>", "Procurar um identificador na rede"),
        ("unpost <id>", "Remover um identificador local"),
        ("status", "Ver status do peer"),
        ("help", "Mostrar esta ajuda"),
        ("clear", "Limpar tela"),
        ("exit", "Sair da aplicação"),
    ]
    
    for cmd, desc in commands:
        print(f"  {Colors.CYAN}{cmd:<25}{Colors.END} {desc}")
    print()


def print_prompt(state):
    """Imprime prompt com informação"""
    status = "ONLINE" if state.joined else "OFFLINE"
    color = Colors.GREEN if state.joined else Colors.RED
    print(f"{color}[{status}]{Colors.END} {Colors.BOLD}p2pnet>{Colors.END} ", end="", flush=True)


def cmd_join(state):
    """Comando join - Entrar na rede"""
    if state.joined:
        print(f"{Colors.YELLOW}[!] Já está na rede.{Colors.END}")
        return
    
    print(f"{Colors.BLUE}[*] Registando no servidor...{Colors.END}")
    
    # Regista no servidor
    result, seqnumber = peer_client_register(state.server_ip, state.server_port, state.tcp_port)
    if result < 0:
        return
    
    state.seqnumber = seqnumber
    
    # Obtém lista de peers
    print(f"{Colors.BLUE}[*] Obtendo lista de peers...{Colors.END}")
    result, peer_list = peer_client_get_peers(state.server_ip, state.server_port)
    if result < 0:
        print(f"{Colors.RED}[✗] Erro ao obter lista de peers.{Colors.END}")
        peer_client_unregister(state.server_ip, state.server_port, state.seqnumber)
        return
    
    peer_count = peer_list.count if peer_list else 0
    print(f"{Colors.BLUE}[*] Encontrados {peer_count} peers na rede.{Colors.END}")
    
    # Estabelece ligações
    connected = establish_connections(state, peer_list) if peer_list else 0
    
    state.joined = True
    
    if connected > 0:
        print(f"{Colors.GREEN}[✓] Ligado à rede com {connected} vizinhos!{Colors.END}")
    elif peer_list and peer_list.count == 0:
        print(f"{Colors.YELLOW}[!] És o primeiro peer na rede.{Colors.END}")
    else:
        print(f"{Colors.YELLOW}[!] Registado mas sem vizinhos conectados.{Colors.END}")


def cmd_leave(state):
    """Comando leave - Sair da rede"""
    if not state.joined:
        print(f"{Colors.YELLOW}[!] Não está na rede.{Colors.END}")
        return
    
    print(f"{Colors.BLUE}[*] Desconectando...{Colors.END}")
    
    # Remove registo
    peer_client_unregister(state.server_ip, state.server_port, state.seqnumber)
    
    # Fecha todas ligações
    for neighbor in state.neighbors.neighbors:
        close_socket(neighbor.socket_fd)
    
    state.neighbors.neighbors.clear()
    state.neighbors.count = 0
    state.joined = False
    
    print(f"{Colors.GREEN}[✓] Desconectado da rede.{Colors.END}")


def cmd_show_neighbors(state):
    """Comando show neighbors - Mostrar vizinhos"""
    if not state.joined:
        print(f"{Colors.YELLOW}[!] Não está na rede.{Colors.END}")
        return
    
    if not state.neighbors.neighbors:
        print(f"{Colors.YELLOW}[!] Nenhum vizinho conectado.{Colors.END}")
        return
    
    external = [n for n in state.neighbors.neighbors if n.is_external]
    internal = [n for n in state.neighbors.neighbors if not n.is_external]
    
    print(f"\n{Colors.BOLD}{Colors.CYAN}Vizinhos Externos ({len(external)}):{Colors.END}")
    if external:
        for n in external:
            print(f"  {Colors.CYAN}→{Colors.END} {n.info.ip}:{n.info.port} "
                  f"{Colors.GRAY}(seq: {n.info.seqnumber}){Colors.END}")
    else:
        print(f"  {Colors.GRAY}(nenhum){Colors.END}")
    
    print(f"\n{Colors.BOLD}{Colors.GREEN}Vizinhos Internos ({len(internal)}):{Colors.END}")
    if internal:
        for n in internal:
            print(f"  {Colors.GREEN}←{Colors.END} {n.info.ip}:{n.info.port} "
                  f"{Colors.GRAY}(seq: {n.info.seqnumber}){Colors.END}")
    else:
        print(f"  {Colors.GRAY}(nenhum){Colors.END}")
    print()


def cmd_release(state, seqnumber):
    """Comando release - Remover vizinho interno"""
    if not state.joined:
        print(f"{Colors.YELLOW}[!] Não está na rede.{Colors.END}")
        return
    
    neighbor = find_neighbor_by_seqnumber(state, seqnumber)
    if not neighbor:
        print(f"{Colors.RED}[✗] Vizinho #{seqnumber} não encontrado.{Colors.END}")
        return
    
    if neighbor.is_external:
        print(f"{Colors.YELLOW}[!] Apenas pode remover vizinhos internos.{Colors.END}")
        return
    
    if remove_neighbor(state, seqnumber) == 0:
        print(f"{Colors.GREEN}[✓] Ligação com peer #{seqnumber} removida.{Colors.END}")
    else:
        print(f"{Colors.RED}[✗] Erro ao remover vizinho.{Colors.END}")


def cmd_list_identifiers(state):
    """Comando list identifiers - Listar identificadores"""
    print(f"\n{Colors.BOLD}{Colors.CYAN}Identificadores Locais ({state.identifiers.count}):{Colors.END}")
    
    if state.identifiers.count == 0:
        print(f"  {Colors.GRAY}(nenhum){Colors.END}")
    else:
        for i, identifier in enumerate(state.identifiers.identifiers, 1):
            print(f"  {Colors.CYAN}{i}.{Colors.END} {identifier}")
    print()


def cmd_post(state, identifier):
    """Comando post - Adicionar identificador"""
    if state.identifiers.add(identifier) == 0:
        print(f"{Colors.GREEN}[✓] Identificador '{identifier}' publicado.{Colors.END}")
    else:
        print(f"{Colors.RED}[✗] Erro: Limite atingido ou já existe.{Colors.END}")


def cmd_search(state, identifier):
    """Comando search - Pesquisar identificador"""
    if not state.joined:
        print(f"{Colors.YELLOW}[!] Não está na rede.{Colors.END}")
        return
    
    # Verifica se já tem
    if state.identifiers.contains(identifier):
        print(f"{Colors.YELLOW}[!] Identificador '{identifier}' já conhece.{Colors.END}")
        return
    
    print(f"{Colors.BLUE}[*] Procurando '{identifier}' na rede...{Colors.END}")
    
    # Pergunta aos vizinhos
    found = False
    for neighbor in state.neighbors.neighbors:
        protocol_query_identifier(neighbor.socket_fd, identifier, state.max_hopcount)
        
        response = tcp_receive(neighbor.socket_fd)
        if response and response.startswith("FND"):
            found = True
            break
    
    if found:
        state.identifiers.add(identifier)
        print(f"{Colors.GREEN}[✓] Identificador '{identifier}' encontrado!{Colors.END}")
    else:
        print(f"{Colors.YELLOW}[!] Identificador '{identifier}' não encontrado.{Colors.END}")


def cmd_unpost(state, identifier):
    """Comando unpost - Remover identificador"""
    if state.identifiers.remove(identifier) == 0:
        print(f"{Colors.GREEN}[✓] Identificador '{identifier}' removido.{Colors.END}")
    else:
        print(f"{Colors.RED}[✗] Identificador não encontrado.{Colors.END}")


def cmd_status(state):
    """Comando status - Mostrar status"""
    print_status(state)


def cmd_exit(state):
    """Comando exit - Sair"""
    print(f"\n{Colors.BLUE}[*] Encerrando...{Colors.END}")
    if state.joined:
        cmd_leave(state)


def process_command(state, command):
    """Processa comando da UI"""
    command = command.strip()
    
    if not command:
        return True
    
    if command == "join":
        cmd_join(state)
    elif command == "leave":
        cmd_leave(state)
    elif command == "show neighbors":
        cmd_show_neighbors(state)
    elif command.startswith("release "):
        try:
            seqnumber = int(command.split()[1])
            cmd_release(state, seqnumber)
        except (ValueError, IndexError):
            print(f"{Colors.YELLOW}Uso: release <seqnumber>{Colors.END}")
    elif command == "list identifiers":
        cmd_list_identifiers(state)
    elif command.startswith("post "):
        identifier = command[5:].strip()
        if identifier:
            cmd_post(state, identifier)
        else:
            print(f"{Colors.YELLOW}Uso: post <identificador>{Colors.END}")
    elif command.startswith("search "):
        identifier = command[7:].strip()
        if identifier:
            cmd_search(state, identifier)
        else:
            print(f"{Colors.YELLOW}Uso: search <identificador>{Colors.END}")
    elif command.startswith("unpost "):
        identifier = command[7:].strip()
        if identifier:
            cmd_unpost(state, identifier)
        else:
            print(f"{Colors.YELLOW}Uso: unpost <identificador>{Colors.END}")
    elif command == "status":
        cmd_status(state)
    elif command == "help":
        print_help()
    elif command == "clear":
        clear_screen()
        print_header()
    elif command == "exit":
        cmd_exit(state)
        return False
    else:
        print(f"{Colors.RED}[✗] Comando desconhecido. Digite 'help' para ajuda.{Colors.END}")
    
    return True


def ui_start(state):
    """Inicia interface de usuário"""
    print_header()
    print_status(state)
    print_help()
    
    running = True
    while running:
        print_prompt(state)
        
        try:
            command = input()
            running = process_command(state, command)
        except KeyboardInterrupt:
            print(f"\n{Colors.BLUE}[*] Interrompido pelo utilizador...{Colors.END}")
            if state.joined:
                cmd_leave(state)
            break
        except EOFError:
            break
    
    if state.joined:
        cmd_leave(state)
    
    print(f"\n{Colors.CYAN}Obrigado por usar P2P Overlay Network!{Colors.END}\n")
