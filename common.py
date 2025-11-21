"""
Definições comuns e estruturas de dados para P2P Overlay Network
"""

# Constantes
MAX_NEIGHBORS = 10
MAX_IDENTIFIERS = 100
MAX_PEERS = 100
BUFFER_SIZE = 4096
MAX_IDENTIFIER_LEN = 64
DEFAULT_SERVER_IP = "192.168.56.21"
DEFAULT_SERVER_PORT = 58000
DEFAULT_HOPCOUNT = 5


class PeerInfo:
    """Informação de um peer"""
    def __init__(self, ip="", port=0, seqnumber=0):
        self.ip = ip
        self.port = port
        self.seqnumber = seqnumber
    
    def __repr__(self):
        return f"PeerInfo(ip={self.ip}, port={self.port}, seqnumber={self.seqnumber})"


class Neighbor:
    """Informação de um vizinho"""
    def __init__(self, peer_info=None, socket_fd=None, is_external=False):
        self.info = peer_info or PeerInfo()
        self.socket_fd = socket_fd
        self.is_external = is_external  # 1 = vizinho externo, 0 = vizinho interno
    
    def __repr__(self):
        return f"Neighbor(info={self.info}, is_external={self.is_external})"


class NeighborList:
    """Lista de vizinhos"""
    def __init__(self):
        self.neighbors = []
        self.count = 0
    
    def add(self, neighbor):
        if self.count < MAX_NEIGHBORS * 2:
            self.neighbors.append(neighbor)
            self.count += 1
            return 0
        return -1
    
    def remove(self, seqnumber):
        for i, neighbor in enumerate(self.neighbors):
            if neighbor.info.seqnumber == seqnumber:
                self.neighbors.pop(i)
                self.count -= 1
                return 0
        return -1
    
    def find_by_seqnumber(self, seqnumber):
        for neighbor in self.neighbors:
            if neighbor.info.seqnumber == seqnumber:
                return neighbor
        return None


class IdentifierList:
    """Lista de identificadores"""
    def __init__(self):
        self.identifiers = []
        self.count = 0
    
    def add(self, identifier):
        if self.count < MAX_IDENTIFIERS:
            if identifier not in self.identifiers:
                self.identifiers.append(identifier)
                self.count += 1
                return 0
        return -1
    
    def remove(self, identifier):
        if identifier in self.identifiers:
            self.identifiers.remove(identifier)
            self.count -= 1
            return 0
        return -1
    
    def contains(self, identifier):
        return identifier in self.identifiers


class PeerList:
    """Lista de peers do servidor"""
    def __init__(self):
        self.peers = []
        self.count = 0
    
    def add(self, peer_info):
        if self.count < MAX_PEERS:
            self.peers.append(peer_info)
            self.count += 1
            return 0
        return -1
    
    def remove(self, seqnumber):
        for i, peer in enumerate(self.peers):
            if peer.seqnumber == seqnumber:
                self.peers.pop(i)
                self.count -= 1
                return 0
        return -1


class PeerState:
    """Estado do peer"""
    def __init__(self):
        self.seqnumber = 0
        self.tcp_port = 0
        self.tcp_server_fd = None
        self.joined = False
        self.neighbors = NeighborList()
        self.identifiers = IdentifierList()
        
        # Configuração
        self.server_ip = DEFAULT_SERVER_IP
        self.server_port = DEFAULT_SERVER_PORT
        self.max_neighbors = 3
        self.max_hopcount = DEFAULT_HOPCOUNT


class PeerServerEntry:
    """Registro no servidor de peers"""
    def __init__(self, ip="", port=0, seqnumber=0):
        self.ip = ip
        self.port = port
        self.seqnumber = seqnumber
    
    def __repr__(self):
        return f"PeerServerEntry(ip={self.ip}, port={self.port}, seqnumber={self.seqnumber})"


def log_message(message):
    """Log de mensagens"""
    print(f"[LOG] {message}")


def error_exit(message):
    """Sair com erro"""
    print(f"[ERRO] {message}")
    exit(1)
