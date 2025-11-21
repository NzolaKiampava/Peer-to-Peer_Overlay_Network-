#!/usr/bin/env python3
"""
Script de teste do P2P Overlay Network (Python)
"""

import os
import sys
import subprocess
import time

def main():
    print("=" * 50)
    print("P2P Overlay Network - Python Quick Test")
    print("=" * 50)
    print()
    
    # Verificar se os arquivos existem
    required_files = ["peer_server.py", "p2pnet.py"]
    for file in required_files:
        if not os.path.exists(file):
            print(f"❌ Erro: {file} não encontrado.")
            print("   Execute este script no diretório python/")
            sys.exit(1)
    
    print("✅ Arquivos Python encontrados")
    print()
    print("Iniciando servidor de peers...")
    print()
    
    try:
        # Iniciar servidor em background
        server_process = subprocess.Popen(
            ["python3", "peer_server.py", "-p", "58000"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )
        
        print(f"Servidor iniciado (PID: {server_process.pid})")
        
        time.sleep(1)
        
        print()
        print("✅ Servidor pronto para conexões")
        print()
        print("Para iniciar peers em outros terminais, execute:")
        print()
        print("Terminal 1 (Peer 1):")
        print("  python3 p2pnet.py -l 5001 -s 127.0.0.1 -p 58000 -n 3 -h 5")
        print()
        print("Terminal 2 (Peer 2):")
        print("  python3 p2pnet.py -l 5002 -s 127.0.0.1 -p 58000 -n 3 -h 5")
        print()
        print("Terminal 3 (Peer 3):")
        print("  python3 p2pnet.py -l 5003 -s 127.0.0.1 -p 58000 -n 3 -h 5")
        print()
        print("Teste interativo:")
        print("  1. Em cada peer, execute: join")
        print("  2. Em um peer, execute: post teste")
        print("  3. Em outro peer, execute: search teste")
        print()
        print("Pressione Ctrl+C para encerrar o servidor")
        print()
        
        # Aguardar entrada
        server_process.wait()
    
    except KeyboardInterrupt:
        print("\n[TEST] A encerrar...")
        server_process.terminate()
        try:
            server_process.wait(timeout=2)
        except subprocess.TimeoutExpired:
            server_process.kill()
    
    except Exception as e:
        print(f"❌ Erro: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()
