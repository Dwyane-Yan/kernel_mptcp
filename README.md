# kernel_mptcp
make

sudo insmod kernel_mptcp.ko

sudo dmesg

[38833.854371] Sent 11 bytes  
[38833.854378] Received: Hello world

sudo rmmod kernel_mptcp
