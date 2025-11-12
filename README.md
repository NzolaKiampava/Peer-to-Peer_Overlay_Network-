# P2P Overlay Network

Implementação de uma rede sobreposta Peer-to-Peer com protocolo de camada de aplicação.

## Estrutura do Projeto

- `include/` - Arquivos header (.h)
- `src/` - Código fonte (.c)
- `obj/` - Arquivos objeto (gerados)
- `bin/` - Executáveis (gerados)

## Compilação

```bash
make
```

Isto gera dois executáveis:
- `bin/peer_server` - Servidor de peers (UDP)
- `bin/p2pnet` - Aplicação peer

## Execução

### 1. Iniciar o Servidor de Peers

```bash
./bin/peer_server [-p port]
```

Exemplo:
```bash
./bin/peer_server -p 58000
```

### 2. Iniciar Peers

```bash
./bin/p2pnet -l <lnkport> [-s addr] [-p prport] [-n neigh] [-h hc]
```

Exemplo:
```bash
./bin/p2pnet -l 5001 -s 192.168.56.21 -p 58000 -n 3 -h 5
```

Parâmetros:
- `-l lnkport` - Porta TCP para escutar ligações (obrigatório)
- `-s addr` - IP do servidor de peers (padrão: 192.168.56.21)
- `-p prport` - Porta do servidor de peers (padrão: 58000)
- `-n neigh` - Número máximo de vizinhos (padrão: 3)
- `-h hc` - Número máximo de saltos (padrão: 5)

## Comandos da Aplicação

- `join` - Entrar na rede
- `leave` - Sair da rede
- `show neighbors` - Mostrar vizinhos
- `release <seqnumber>` - Remover vizinho interno
- `list identifiers` - Listar identificadores
- `post <id>` - Adicionar identificador
- `search <id>` - Pesquisar identificador
- `unpost <id>` - Remover identificador
- `exit` - Sair

## Testes com Vagrant

```bash
# Iniciar VMs
vagrant up

# SSH para servidor
vagrant ssh peer_server

# SSH para peers
vagrant ssh peer1
vagrant ssh peer2
vagrant ssh peer3
```

## Divisão de Trabalho

### Membro 1: Servidor de Peers
- `peer_server.h/c`
- `peer_server_main.c`
- Protocolo UDP

### Membro 2: Protocolos Overlay
- `protocol.h/c`
- `network.c` (TCP)
- Gestão de ligações

### Membro 3: Interface e Cliente
- `ui.h/c`
- `peer_client.h/c`
- `p2pnet_main.c`

## Limpeza

```bash
make clean
``` 
