"""
Programa principal do peer P2P
"""

import threading
import argparse
from common import PeerState, DEFAULT_SERVER_IP, DEFAULT_SERVER_PORT, DEFAULT_HOPCOUNT
from network import create_tcp_server, tcp_accept, tcp_receive, close_socket
from protocol import protocol_handle_link
from ui import ui_start


def accept_connections_thread(state):
    """Thread para aceitar conexões TCP"""
    while state.joined:
        try:
            client_fd, client_ip = tcp_accept(state.tcp_server_fd)
            
            if client_fd is None:
                continue
            
            # Recebe mensagem
            buffer = tcp_receive(client_fd)
            if buffer:
                print(f"[CONEXÃO] Recebido: {buffer}")
                protocol_handle_link(state, client_fd, buffer, client_ip)
            else:
                close_socket(client_fd)
        
        except Exception as e:
            print(f"[ERRO] Erro ao aceitar conexão: {e}")
            continue


def main():
    """Programa principal do peer"""
    
    # Parse argumentos
    parser = argparse.ArgumentParser(description="P2P Overlay Network - Peer", add_help=True)
    parser.add_argument("-l", "--lnkport", type=int, required=True,
                        help="Porta TCP para escutar ligações (obrigatório)")
    parser.add_argument("-s", "--server", type=str, default=DEFAULT_SERVER_IP,
                        help=f"IP do servidor de peers (padrão: {DEFAULT_SERVER_IP})")
    parser.add_argument("-p", "--port", type=int, default=DEFAULT_SERVER_PORT,
                        help=f"Porta do servidor de peers (padrão: {DEFAULT_SERVER_PORT})")
    parser.add_argument("-n", "--neighbors", type=int, default=3,
                        help="Número máximo de vizinhos (padrão: 3)")
    parser.add_argument("-hc", "--hopcount", type=int, default=DEFAULT_HOPCOUNT,
                        help=f"Número máximo de saltos (padrão: {DEFAULT_HOPCOUNT})")
    
    args = parser.parse_args()
    
    # Validações
    if args.lnkport <= 0 or args.lnkport > 65535:
        print("Erro: porta TCP deve estar entre 1 e 65535")
        return
    
    if args.neighbors <= 0:
        print("Erro: número de vizinhos deve ser > 0")
        return
    
    if args.hopcount <= 0:
        print("Erro: hopcount deve ser > 0")
        return
    
    # Cria estado do peer
    state = PeerState()
    state.server_ip = args.server
    state.server_port = args.port
    state.tcp_port = args.lnkport
    state.max_neighbors = args.neighbors
    state.max_hopcount = args.hopcount
    
    # Cria servidor TCP
    state.tcp_server_fd = create_tcp_server(args.lnkport)
    if not state.tcp_server_fd:
        print(f"Erro ao criar servidor TCP na porta {args.lnkport}")
        return
    
    print("P2P Overlay Network")
    print(f"Servidor TCP na porta {state.tcp_port}")
    print(f"Servidor de peers: {state.server_ip}:{state.server_port}")
    print(f"Max vizinhos: {state.max_neighbors}, Max hopcount: {state.max_hopcount}")
    print()
    
    # Thread para aceitar conexões
    accept_thread = threading.Thread(target=accept_connections_thread, args=(state,), daemon=True)
    accept_thread.start()
    
    # Interface de usuário
    try:
        ui_start(state)
    except KeyboardInterrupt:
        print("\n[MAIN] A sair...")
    finally:
        # Cleanup
        state.joined = False
        close_socket(state.tcp_server_fd)


if __name__ == "__main__":
    main()
