# Projeto P2P Overlay Network - Implementação Completa

## ✅ Projeto Criado com Sucesso!

Todos os arquivos necessários para o projeto P2P Overlay Network foram criados e organizados corretamente.

## 📁 Estrutura de Diretórios Criada

```
Peer-to-Peer_Overlay_Network-/
├── include/                    # Headers (.h)
│   ├── common.h               # Definições comuns e estruturas
│   ├── network.h              # Interface de rede (TCP/UDP)
│   ├── peer_server.h          # Interface do servidor de peers
│   ├── peer_client.h          # Interface do cliente de peers
│   ├── protocol.h             # Protocolos overlay
│   └── ui.h                   # Interface de usuário
│
├── src/                        # Código-fonte (.c)
│   ├── network.c              # Implementação de rede
│   ├── peer_server.c          # Implementação do servidor
│   ├── peer_server_main.c     # Programa principal do servidor
│   ├── peer_client.c          # Implementação do cliente
│   ├── protocol.c             # Implementação dos protocolos
│   ├── ui.c                   # Implementação da interface
│   └── p2pnet_main.c          # Programa principal do peer
│
├── scripts/                    # Scripts auxiliares
│   └── setup.sh               # Script de instalação
│
├── Makefile                    # Build system
├── Vagrantfile                 # Configuração Vagrant
└── README.md                   # Documentação
```

## 🛠️ Compilação

Para compilar o projeto em um ambiente Linux/Unix:

```bash
make
```

Isto gerará dois executáveis:
- `bin/peer_server` - Servidor de peers
- `bin/p2pnet` - Aplicação peer

## 🚀 Execução

### 1. Iniciar o Servidor de Peers

```bash
./bin/peer_server -p 58000
```

### 2. Iniciar Peers (em outros terminais)

```bash
# Peer 1
./bin/p2pnet -l 5001 -s 127.0.0.1 -p 58000 -n 3 -h 5

# Peer 2
./bin/p2pnet -l 5002 -s 127.0.0.1 -p 58000 -n 3 -h 5

# Peer 3
./bin/p2pnet -l 5003 -s 127.0.0.1 -p 58000 -n 3 -h 5
```

## 📝 Comandos da Aplicação

- `join` - Entrar na rede
- `leave` - Sair da rede
- `show neighbors` - Mostrar vizinhos (internos e externos)
- `release <seqnumber>` - Remover vizinho interno
- `list identifiers` - Listar identificadores conhecidos
- `post <id>` - Adicionar um novo identificador
- `search <id>` - Pesquisar um identificador na rede
- `unpost <id>` - Remover um identificador
- `help` - Mostrar ajuda
- `exit` - Sair da aplicação

## 🏗️ Arquitetura

### Componentes Principais

1. **Servidor de Peers (peer_server)**
   - Gerencia registro/desregisto de peers
   - Responde a queries UDP
   - Mantém lista global de peers ativos

2. **Peer (p2pnet)**
   - Registra-se no servidor de peers
   - Estabelece ligações TCP com outros peers
   - Implementa protocolo overlay
   - Fornece interface interativa

3. **Protocolos Implementados**
   - **LNK**: Ligação entre peers
   - **FRC**: Força de ligação (para peers de maior seqnumber)
   - **QRY**: Query de identificadores
   - **FND/NOTFND**: Resposta a queries

## 🔄 Protocolos de Comunicação

### UDP (Servidor de Peers)
- **REG <porta>**: Registar novo peer
- **UNR <seqnumber>**: Desregistar peer
- **PEERS**: Obter lista de peers ativos

### TCP (Entre Peers)
- **LNK <seqnumber>**: Pedido de ligação
- **FRC <seqnumber>**: Força de ligação
- **CNF**: Confirmação de ligação
- **QRY <id> <hopcount>**: Query de identificador
- **FND <id>**: Identificador encontrado
- **NOTFND <id>**: Identificador não encontrado

## 🧪 Teste com Vagrant

```bash
# Iniciar VMs
vagrant up

# Acessar servidor
vagrant ssh peer_server
cd /home/vagrant/p2pnet
make
./bin/peer_server -p 58000

# Acessar peers (em outros terminais)
vagrant ssh peer1
cd /home/vagrant/p2pnet
make
./bin/p2pnet -l 5001

# Repetir para peer2 e peer3
```

## 📚 Divisão de Trabalho Sugerida

### Membro 1: Servidor de Peers
- `peer_server.h / peer_server.c`
- `peer_server_main.c`
- Comunicação UDP

### Membro 2: Protocolos Overlay
- `protocol.h / protocol.c`
- Parte TCP de `network.c`
- Gestão de vizinhos

### Membro 3: Interface e Cliente
- `peer_client.h / peer_client.c`
- `ui.h / ui.c`
- `p2pnet_main.c`
- Integração final

## 🔧 Dependências

- **GCC** (compilador C)
- **Make** (build system)
- **Pthreads** (threads)
- **Standard POSIX sockets**

## 📋 Checklist de Implementação

- ✅ Headers com estruturas de dados
- ✅ Implementação de rede (TCP/UDP)
- ✅ Servidor de peers com UDP
- ✅ Cliente de peers
- ✅ Protocolos overlay (LNK, FRC, QRY)
- ✅ Interface de usuário
- ✅ Makefiles de compilação
- ✅ Vagrantfile para testes
- ✅ Documentação completa

## 🛑 Limpar Compilação

```bash
make clean
```

Isto remove os diretórios `obj/` e `bin/`.

---

**Projeto pronto para desenvolvimento e testes!** 🎉
