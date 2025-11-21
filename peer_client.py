"""
Cliente para comunicação com servidor de peers
"""

from network import create_udp_socket, udp_send, udp_receive, close_socket
from common import PeerList, PeerInfo, DEFAULT_SERVER_IP, DEFAULT_SERVER_PORT


def peer_client_register(server_ip, server_port, tcp_port):
    """Registra peer no servidor e retorna seqnumber"""
    sockfd = create_udp_socket()
    if not sockfd:
        print(f"[ERRO] Não foi possível criar socket UDP")
        return -1, None
    
    try:
        message = f"REG {tcp_port}"
        
        if udp_send(sockfd, server_ip, server_port, message) < 0:
            close_socket(sockfd)
            return -1, None
        
        response, _ = udp_receive(sockfd)
        
        if not response:
            print(f"[ERRO] Servidor de peers ({server_ip}:{server_port}) não respondeu")
            close_socket(sockfd)
            return -1, None
        
        if response.startswith("SQN "):
            try:
                seqnumber = int(response.split()[1])
                close_socket(sockfd)
                print(f"[CLIENTE] Registado com seqnumber {seqnumber}")
                return 0, seqnumber
            except (ValueError, IndexError):
                pass
        
        close_socket(sockfd)
        print(f"[ERRO] Resposta inválida do servidor: {response}")
        return -1, None
    
    except Exception as e:
        print(f"[ERRO] Ao registar: {e}")
        close_socket(sockfd)
        return -1, None


def peer_client_unregister(server_ip, server_port, seqnumber):
    """Remove registo do peer"""
    sockfd = create_udp_socket()
    if not sockfd:
        return -1
    
    try:
        message = f"UNR {seqnumber}"
        
        if udp_send(sockfd, server_ip, server_port, message) < 0:
            close_socket(sockfd)
            return -1
        
        response, _ = udp_receive(sockfd)
        
        close_socket(sockfd)
        
        if response == "OK":
            return 0
        return -1
    
    except Exception as e:
        print(f"Erro ao desregistar: {e}")
        close_socket(sockfd)
        return -1


def peer_client_get_peers(server_ip, server_port):
    """Obtém lista de peers ativos"""
    sockfd = create_udp_socket()
    if not sockfd:
        return -1, None
    
    try:
        if udp_send(sockfd, server_ip, server_port, "PEERS") < 0:
            close_socket(sockfd)
            return -1, None
        
        response, _ = udp_receive(sockfd)
        
        close_socket(sockfd)
        
        if not response:
            print(f"[ERRO] Servidor não respondeu à query PEERS")
            return -1, None
        
        # Parse resposta
        peer_list = PeerList()
        lines = response.split("\n")
        
        if not lines or not lines[0].startswith("LST"):
            print(f"[ERRO] Resposta inválida: {response[:50]}...")
            return -1, None
        
        for line in lines[1:]:
            line = line.strip()
            if not line:
                continue
            
            try:
                parts = line.split("#")
                if len(parts) != 2:
                    continue
                
                ip_port = parts[0].split(":")
                if len(ip_port) != 2:
                    continue
                
                ip = ip_port[0]
                port = int(ip_port[1])
                seqnum = int(parts[1])
                
                peer_info = PeerInfo(ip=ip, port=port, seqnumber=seqnum)
                peer_list.add(peer_info)
                
                if peer_list.count >= 100:  # MAX_PEERS
                    break
            
            except (ValueError, IndexError):
                continue
        
        return 0, peer_list
    
    except Exception as e:
        print(f"Erro ao obter lista de peers: {e}")
        close_socket(sockfd)
        return -1, None
