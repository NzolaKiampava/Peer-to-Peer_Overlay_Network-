#!/bin/bash

# Setup script para P2P Overlay Network
# Este script configura o ambiente de desenvolvimento

echo "=== Setup P2P Overlay Network ==="

# Instalar dependências (para sistemas Ubuntu/Debian)
if command -v apt-get &> /dev/null; then
    echo "Instalando dependências..."
    apt-get update
    apt-get install -y build-essential gcc make gdb
fi

# Compilar projeto
echo "Compilando projeto..."
make

# Criar testes
echo "Projeto compilado com sucesso!"
echo ""
echo "Para iniciar o servidor de peers:"
echo "  ./bin/peer_server -p 58000"
echo ""
echo "Para iniciar um peer:"
echo "  ./bin/p2pnet -l 5001 -s 192.168.56.21 -p 58000 -n 3 -h 5"
echo ""
echo "Comandos disponíveis no peer:"
echo "  join            - Entrar na rede"
echo "  leave           - Sair da rede"
echo "  show neighbors  - Mostrar vizinhos"
echo "  list identifiers - Listar identificadores"
echo "  post <id>       - Adicionar identificador"
echo "  search <id>     - Pesquisar identificador"
echo "  help            - Mostrar ajuda"
echo "  exit            - Sair"
