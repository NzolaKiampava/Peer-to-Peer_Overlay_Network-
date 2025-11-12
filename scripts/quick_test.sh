#!/bin/bash

# Script de teste rápido do P2P Overlay Network
# Executa um teste básico com servidor e 3 peers

echo "=========================================="
echo "P2P Overlay Network - Quick Test"
echo "=========================================="
echo ""

# Verificar se os executáveis existem
if [ ! -f "bin/peer_server" ] || [ ! -f "bin/p2pnet" ]; then
    echo "❌ Erro: Executáveis não encontrados."
    echo "   Execute 'make' primeiro."
    exit 1
fi

echo "✅ Executáveis encontrados"
echo ""
echo "Iniciando servidor de peers..."
echo ""

# Iniciar servidor em background
./bin/peer_server -p 58000 &
SERVER_PID=$!

sleep 1

echo "Servidor iniciado (PID: $SERVER_PID)"
echo ""
echo "✅ Servidor pronto para conexões"
echo ""
echo "Para iniciar peers em outros terminais, execute:"
echo ""
echo "Terminal 1 (Peer 1):"
echo "  ./bin/p2pnet -l 5001 -s 127.0.0.1 -p 58000 -n 3 -h 5"
echo ""
echo "Terminal 2 (Peer 2):"
echo "  ./bin/p2pnet -l 5002 -s 127.0.0.1 -p 58000 -n 3 -h 5"
echo ""
echo "Terminal 3 (Peer 3):"
echo "  ./bin/p2pnet -l 5003 -s 127.0.0.1 -p 58000 -n 3 -h 5"
echo ""
echo "Teste interativo:"
echo "  1. Em cada peer, execute: join"
echo "  2. Em um peer, execute: post teste"
echo "  3. Em outro peer, execute: search teste"
echo ""
echo "Pressione Ctrl+C para encerrar o servidor"
echo ""

# Aguardar terminação
wait $SERVER_PID
