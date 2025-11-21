"""
Versão Python do P2P Overlay Network

Este diretório contém a implementação em Python do projeto P2P Overlay Network.

Arquivos:
- common.py      : Definições comuns e estruturas de dados
- network.py     : Implementação de rede (TCP/UDP)
- peer_server.py : Servidor de peers (UDP)
- peer_client.py : Cliente para servidor de peers
- protocol.py    : Protocolos overlay (LNK, FRC, QRY)
- ui.py          : Interface de usuário
- p2pnet.py      : Programa principal do peer

Requisitos:
- Python 3.6+
- Nenhuma dependência externa (apenas módulos padrão)

Execução:

1. Iniciar o servidor de peers:
   python3 peer_server.py -p 58000

2. Iniciar peers (em outros terminais):
   python3 p2pnet.py -l 5001 -s 127.0.0.1 -p 58000 -n 3 -h 5
   python3 p2pnet.py -l 5002 -s 127.0.0.1 -p 58000 -n 3 -h 5
   python3 p2pnet.py -l 5003 -s 127.0.0.1 -p 58000 -n 3 -h 5

Comandos disponíveis:
- join                  : Entrar na rede
- leave                 : Sair da rede
- show neighbors        : Mostrar vizinhos
- release <seqnumber>   : Remover vizinho interno
- list identifiers      : Listar identificadores
- post <id>             : Adicionar identificador
- search <id>           : Pesquisar identificador na rede
- unpost <id>           : Remover identificador
- help                  : Mostrar ajuda
- exit                  : Sair da aplicação
"""
