"""
Implementação dos protocolos overlay (LNK, FRC, QRY)
"""

from network import tcp_send, tcp_receive, close_socket
from peer_client import peer_client_get_peers
from common import PeerInfo, Neighbor


def protocol_link_request(sockfd, my_seqnumber):
    """Envia pedido LNK"""
    message = f"LNK {my_seqnumber}\n"
    return tcp_send(sockfd, message)


def protocol_force_request(sockfd, my_seqnumber):
    """Envia pedido FRC"""
    message = f"FRC {my_seqnumber}\n"
    return tcp_send(sockfd, message)


def protocol_handle_link(state, client_fd, message, client_ip):
    """Trata pedido de ligação (LNK/FRC)"""
    try:
        parts = message.strip().split()
        
        if not parts:
            return -1
        
        is_force = False
        if parts[0] == "LNK":
            is_force = False
        elif parts[0] == "FRC":
            is_force = True
        else:
            return -1
        
        if len(parts) < 2:
            return -1
        
        client_seqnumber = int(parts[1])
        
        # Conta vizinhos internos
        internal_count = count_internal_neighbors(state)
        
        # Pode aceitar a ligação
        if internal_count < state.max_neighbors:
            tcp_send(client_fd, "CNF\n")
            
            peer_info = PeerInfo(ip=client_ip, port=0, seqnumber=client_seqnumber)
            add_neighbor(state, peer_info, client_fd, is_external=False)
            
            print(f"[PROTOCOLO] Ligação aceite de peer {client_seqnumber}")
            return 0
        
        # Se FRC e tem vizinho com seqnumber maior
        if is_force:
            higher = find_internal_with_higher_seqnumber(state, client_seqnumber)
            if higher:
                tcp_send(client_fd, "CNF\n")
                
                peer_info = PeerInfo(ip=client_ip, port=0, seqnumber=client_seqnumber)
                
                # Remove vizinho com seqnumber maior
                removed_seq = higher.info.seqnumber
                remove_neighbor(state, removed_seq)
                
                # Adiciona novo vizinho
                add_neighbor(state, peer_info, client_fd, is_external=False)
                
                print(f"[PROTOCOLO] Ligação FRC aceite de peer {client_seqnumber}, "
                      f"removido peer {removed_seq}")
                return 0
        
        # Não pode aceitar
        close_socket(client_fd)
        return -1
    
    except (ValueError, IndexError) as e:
        print(f"Erro ao processar ligação: {e}")
        return -1


def protocol_query_identifier(sockfd, identifier, hopcount):
    """Envia query de identificador"""
    message = f"QRY {identifier} {hopcount}\n"
    return tcp_send(sockfd, message)


def protocol_handle_query(state, sockfd, message):
    """Trata query de identificador"""
    try:
        parts = message.strip().split()
        
        if len(parts) < 3 or parts[0] != "QRY":
            return -1
        
        identifier = parts[1]
        hopcount = int(parts[2])
        
        # Verifica se tem identificador
        if state.identifiers.contains(identifier):
            response = f"FND {identifier}\n"
            tcp_send(sockfd, response)
            return 1
        
        # Se hopcount > 1, pergunta aos vizinhos
        if hopcount > 1:
            for neighbor in state.neighbors.neighbors:
                protocol_query_identifier(neighbor.socket_fd, identifier, hopcount - 1)
                
                response = tcp_receive(neighbor.socket_fd)
                if response and response.startswith("FND"):
                    fwd = f"FND {identifier}\n"
                    tcp_send(sockfd, fwd)
                    return 1
        
        response = f"NOTFND {identifier}\n"
        tcp_send(sockfd, response)
        return 0
    
    except (ValueError, IndexError) as e:
        print(f"Erro ao processar query: {e}")
        return -1


def add_neighbor(state, peer, sockfd, is_external=False):
    """Adiciona vizinho"""
    if state.neighbors.count >= 20:  # MAX_NEIGHBORS * 2
        return -1
    
    neighbor = Neighbor(peer_info=peer, socket_fd=sockfd, is_external=is_external)
    return state.neighbors.add(neighbor)


def remove_neighbor(state, seqnumber):
    """Remove vizinho por seqnumber"""
    neighbor = state.neighbors.find_by_seqnumber(seqnumber)
    if neighbor:
        close_socket(neighbor.socket_fd)
        return state.neighbors.remove(seqnumber)
    return -1


def count_external_neighbors(state):
    """Conta vizinhos externos"""
    return sum(1 for n in state.neighbors.neighbors if n.is_external)


def count_internal_neighbors(state):
    """Conta vizinhos internos"""
    return sum(1 for n in state.neighbors.neighbors if not n.is_external)


def find_neighbor_by_seqnumber(state, seqnumber):
    """Encontra vizinho por seqnumber"""
    return state.neighbors.find_by_seqnumber(seqnumber)


def find_internal_with_higher_seqnumber(state, seqnumber):
    """Encontra vizinho interno com seqnumber maior"""
    for neighbor in state.neighbors.neighbors:
        if not neighbor.is_external and neighbor.info.seqnumber > seqnumber:
            return neighbor
    return None


def establish_connections(state, peer_list):
    """Estabelece ligações iniciais"""
    connected = 0
    
    for peer in peer_list.peers:
        if connected >= state.max_neighbors:
            break
        
        # Apenas conecta com peers de seqnumber menor
        if peer.seqnumber >= state.seqnumber:
            continue
        
        sockfd = tcp_connect(peer.ip, peer.port)
        if not sockfd:
            continue
        
        if protocol_link_request(sockfd, state.seqnumber) < 0:
            close_socket(sockfd)
            continue
        
        response = tcp_receive(sockfd)
        if not response:
            close_socket(sockfd)
            continue
        
        if response.startswith("CNF"):
            add_neighbor(state, peer, sockfd, is_external=True)
            connected += 1
            print(f"[CONEXÃO] Ligado ao peer {peer.seqnumber}")
        else:
            close_socket(sockfd)
    
    return connected


def reconnect_after_disconnect(state):
    """Reconecta após desconexão"""
    if count_external_neighbors(state) > 0:
        return 0
    
    result, peer_list = peer_client_get_peers(state.server_ip, state.server_port)
    if result < 0 or not peer_list:
        return -1
    
    connected = establish_connections(state, peer_list)
    
    # Se ainda não conseguiu, tenta FRC
    if connected == 0:
        for peer in peer_list.peers:
            if peer.seqnumber >= state.seqnumber:
                continue
            
            sockfd = tcp_connect(peer.ip, peer.port)
            if not sockfd:
                continue
            
            if protocol_force_request(sockfd, state.seqnumber) < 0:
                close_socket(sockfd)
                continue
            
            response = tcp_receive(sockfd)
            if response and response.startswith("CNF"):
                add_neighbor(state, peer, sockfd, is_external=True)
                connected += 1
                print(f"[RECONEXÃO] Ligado ao peer {peer.seqnumber} com FRC")
                break
            
            close_socket(sockfd)
    
    return connected


# Importa tcp_connect
from network import tcp_connect
