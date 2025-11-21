"""
Implementação de funções de rede (TCP/UDP)
"""

import socket
import struct

BUFFER_SIZE = 4096


def create_udp_socket():
    """Cria socket UDP"""
    try:
        sockfd = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sockfd.settimeout(3.0)  # 3 segundos de timeout
        return sockfd
    except OSError as e:
        print(f"Erro ao criar socket UDP: {e}")
        return None


def udp_send(sockfd, ip, port, message):
    """Envia mensagem UDP"""
    try:
        sockfd.sendto(message.encode(), (ip, port))
        return len(message)
    except OSError as e:
        print(f"Erro ao enviar UDP: {e}")
        return -1


def udp_receive(sockfd, buffer_size=BUFFER_SIZE):
    """Recebe mensagem UDP"""
    try:
        data, addr = sockfd.recvfrom(buffer_size)
        return data.decode(), addr
    except socket.timeout:
        return None, None
    except OSError as e:
        print(f"Erro ao receber UDP: {e}")
        return None, None


def tcp_connect(ip, port):
    """Conecta via TCP"""
    try:
        sockfd = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sockfd.connect((ip, port))
        return sockfd
    except OSError as e:
        print(f"Erro ao conectar TCP: {e}")
        return None


def tcp_send(sockfd, message):
    """Envia mensagem TCP"""
    try:
        if isinstance(message, str):
            message = message.encode()
        sockfd.sendall(message)
        return len(message)
    except OSError as e:
        print(f"Erro ao enviar TCP: {e}")
        return -1


def tcp_receive(sockfd, buffer_size=BUFFER_SIZE):
    """Recebe mensagem TCP"""
    try:
        data = sockfd.recv(buffer_size)
        if not data:
            return None
        return data.decode()
    except OSError as e:
        print(f"Erro ao receber TCP: {e}")
        return None


def create_tcp_server(port):
    """Cria servidor TCP"""
    try:
        sockfd = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sockfd.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sockfd.bind(("0.0.0.0", port))
        sockfd.listen(10)
        return sockfd
    except OSError as e:
        print(f"Erro ao criar servidor TCP: {e}")
        return None


def tcp_accept(server_fd):
    """Aceita conexão TCP"""
    try:
        client_fd, addr = server_fd.accept()
        client_ip = addr[0]
        return client_fd, client_ip
    except OSError as e:
        print(f"Erro ao aceitar conexão: {e}")
        return None, None


def close_socket(sockfd):
    """Fecha socket"""
    if sockfd is not None:
        try:
            sockfd.close()
        except OSError:
            pass


def set_socket_nonblocking(sockfd):
    """Define socket como não-bloqueante"""
    try:
        sockfd.setblocking(False)
        return 0
    except OSError as e:
        print(f"Erro ao configurar socket não-bloqueante: {e}")
        return -1
