# -*- mode: ruby -*-
# vi: set ft=ruby :

Vagrant.configure("2") do |config|
  config.ssh.insert_key = false
  # Se não tiveres o plugin vagrant-vbguest, comenta a linha abaixo:
  # config.vbguest.auto_update = false

  # Servidor de Peers
  config.vm.define "peer_server" do |server|
    server.vm.box = "ubuntu/trusty64"
    server.vm.hostname = "peer-server"
    server.vm.network "private_network", ip: "192.168.56.21"
    server.vm.provider "virtualbox" do |vb|
      vb.name = "peer-server"
      vb.customize ["modifyvm", :id, "--natdnshostresolver1", "on"]
      vb.memory = "512"
    end
    # Python já está pré-instalado
  end

  # Peer 1
  config.vm.define "peer1" do |peer|
    peer.vm.box = "ubuntu/trusty64"
    peer.vm.hostname = "peer1"
    peer.vm.network "private_network", ip: "192.168.56.10"
    peer.vm.provider "virtualbox" do |vb|
      vb.name = "peer1"
      vb.customize ["modifyvm", :id, "--natdnshostresolver1", "on"]
      vb.memory = "512"
    end
    # Python já está pré-instalado
  end

  # Peer 2
  config.vm.define "peer2" do |peer|
    peer.vm.box = "ubuntu/trusty64"
    peer.vm.hostname = "peer2"
    peer.vm.network "private_network", ip: "192.168.56.11"
    peer.vm.provider "virtualbox" do |vb|
      vb.name = "peer2"
      vb.customize ["modifyvm", :id, "--natdnshostresolver1", "on"]
      vb.memory = "512"
    end
    # Python já está pré-instalado
  end

  # Sincronização de pasta
  config.vm.synced_folder ".", "/home/vagrant/p2pnet"
end
