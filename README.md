# BootMe
Library and Command line utility for booting boots/dhcp compliant systems with a specified boot image

# Testing with VMs
Rather than comandeer hardware, it is easier to test that this is working using qemu and a local
network bridge.

First we create a network bridge and give it a static IP:
```shell
# ip link add name virbr0 type bridge
# ip addess add 192.168.111.1/24 dev virbr0
# ip link set dev virbr0 up
```

Then we create and add a TAP device to that bridge:
```shell
# ip tuntap add dev tap0 mode tap user `whoami`
# ip link set tap0 master virbr0
```

And finally we can boot a VM from that TAP device (note that this vm is an appliance, it has no
disk and must pull kernel and initramfs from BootMe).
```shell
qemu-system-x86_64 \
-machine pc-q35-6.2 \
-accel kvm \
-cpu host,migratable=on \
-m 512 \
-serial stdio \
-smp 1 \
-device pcie-root-port,port=16,chassis=1,id=pci.1,bus=pcie.0,multifunction=on,addr=0x2 \
-netdev tap,ifname=tap0,id=hostnet0 \
-device virtio-net-pci,netdev=hostnet0,id=net0,mac=52:54:00:82:c1:3a,bus=pci.1,addr=0x0,bootindex=1
```
