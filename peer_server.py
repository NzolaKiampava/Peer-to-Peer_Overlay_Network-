"""
Implementação do Servidor de Peers (UDP)
"""

import socket
import threading
from network import create_udp_socket, udp_receive, close_socket
from common import PeerServerEntry, PeerList, DEFAULT_SERVER_PORT, BUFFER_SIZE


class PeerServer:
    """Servidor de peers"""
    
    def __init__(self, port=DEFAULT_SERVER_PORT):
        self.entries = []
        self.count = 0
        self.next_seqnumber = 1
        self.port = port
        self.sockfd = create_udp_socket()
        self.running = True
        
        if self.sockfd:
            try:
                self.sockfd.bind(("0.0.0.0", port))
                print(f"[SERVIDOR] Servidor de peers iniciado na porta {port}")
            except OSError as e:
                print(f"[ERRO] Erro ao iniciar servidor: {e}")
                self.sockfd = None
    
    def handle_register(self, message, client_addr, response_list):
        """Handler REG - Registar novo peer"""
        try:
            parts = message.split()
            if len(parts) < 2:
                response_list.append(("NOK", client_addr))
                return
            
            lnkport = int(parts[1])
            
            if self.count >= 100:  # MAX_PEERS
                response_list.append(("NOK", client_addr))
                return
            
            # Adiciona peer
            entry = PeerServerEntry(
                ip=client_addr[0],
                port=lnkport,
                seqnumber=self.next_seqnumber
            )
            
            self.entries.append(entry)
            self.count += 1
            
            response = f"SQN {self.next_seqnumber}"
            response_list.append((response, client_addr))
            
            print(f"[REG] Peer {entry.ip}:{entry.port} registrado com seqnumber {entry.seqnumber}")
            self.next_seqnumber += 1
            
        except (ValueError, IndexError) as e:
            print(f"[ERRO] Erro ao registar peer: {e}")
            response_list.append(("NOK", client_addr))
    
    def handle_unregister(self, message, client_addr, response_list):
        """Handler UNR - Desregistar peer"""
        try:
            parts = message.split()
            if len(parts) < 2:
                response_list.append(("NOK", client_addr))
                return
            
            seqnumber = int(parts[1])
            
            # Procura peer
            found = -1
            for i, entry in enumerate(self.entries):
                if entry.seqnumber == seqnumber:
                    found = i
                    break
            
            if found < 0:
                response_list.append(("NOK", client_addr))
                return
            
            # Remove peer
            self.entries.pop(found)
            self.count -= 1
            
            response_list.append(("OK", client_addr))
            print(f"[UNR] Peer com seqnumber {seqnumber} removido")
            
        except (ValueError, IndexError) as e:
            print(f"[ERRO] Erro ao desregistar peer: {e}")
            response_list.append(("NOK", client_addr))
    
    def handle_peers_list(self, client_addr, response_list):
        """Handler PEERS - Listar peers ativos"""
        response = "LST\n"
        
        for entry in self.entries:
            response += f"{entry.ip}:{entry.port}#{entry.seqnumber}\n"
        
        response += "\n"
        response_list.append((response, client_addr))
    
    def start(self):
        """Inicia servidor"""
        if not self.sockfd:
            print("[ERRO] Socket não configurado")
            return
        
        print("[SERVIDOR] Aguardando requisições...")
        
        try:
            while self.running:
                try:
                    buffer, addr = udp_receive(self.sockfd)
                    
                    if buffer is None:
                        continue
                    
                    print(f"[RECV] {buffer}")
                    
                    response_list = []
                    
                    # Processa comando
                    if buffer.startswith("REG "):
                        self.handle_register(buffer, addr, response_list)
                    elif buffer.startswith("UNR "):
                        self.handle_unregister(buffer, addr, response_list)
                    elif buffer == "PEERS":
                        self.handle_peers_list(addr, response_list)
                    else:
                        response_list.append(("NOK", addr))
                    
                    # Envia resposta
                    for response, client_addr in response_list:
                        self.sockfd.sendto(response.encode(), client_addr)
                        print(f"[SEND] {response.strip()}")
                
                except KeyboardInterrupt:
                    break
                except Exception as e:
                    print(f"[ERRO] Erro ao processar requisição: {e}")
        
        finally:
            self.stop()
    
    def stop(self):
        """Encerra servidor"""
        self.running = False
        close_socket(self.sockfd)
        print("[SERVIDOR] Servidor encerrado")


def main():
    """Programa principal do servidor"""
    import argparse
    
    parser = argparse.ArgumentParser(description="Servidor de Peers P2P")
    parser.add_argument("-p", "--port", type=int, default=DEFAULT_SERVER_PORT,
                        help=f"Porta do servidor (padrão: {DEFAULT_SERVER_PORT})")
    
    args = parser.parse_args()
    
    server = PeerServer(args.port)
    if server.sockfd:
        try:
            server.start()
        except KeyboardInterrupt:
            print("\n[SERVIDOR] A encerrar...")


if __name__ == "__main__":
    from common import DEFAULT_SERVER_PORT
    main()
