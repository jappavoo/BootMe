#!/usr/bin/python3


import argparse
import shutil
import subprocess
import os
import tempfile


SEP='$'


def check_file(afile):
    try:
        os.stat(afile)
    except:
        print("Failed to find required file: {}, please use --pxe-src to specify a source directory".format(afile))
        sys.exit(1)


def main():
    parser = argparse.ArgumentParser(description="dnsmasq configuration generator")
    parser.add_argument('--iface', type=str, help='Have dnsmasq bind to the specified interface only')
    parser.add_argument('--dnsmasq-exe', type=str, help='/path/to/dnsmasq to use instead of relying on $PATH')
    parser.add_argument('--dhcp-net', type=str, default='172.16.1', help='The first three octets of a C class network to use for dhcp')
    parser.add_argument('--pxe-src', type=str, default='./pxebin', help='/path/to/pxebin where we can find pxelinux.0 and menu.c32')
    parser.add_argument('--tftp-target', type=str, default='ukl', help='The target name and subdirectory for TFTP')
    parser.add_argument('host', type=str, nargs='+', help='An entry to be added to boot a host in the form of "mac_addr{sep}path/to/kernel{sep}kernelCmdLine{sep}path/to/initrd" where the kernel command line and initrd path are optional'.format(sep=SEP))

    args = parser.parse_args()

    check_file('{}/pxelinux.0'.format(args.pxe_src))
    check_file('{}/menu.c32'.format(args.pxe_src))
    check_file('{}/ldlinux.c32'.format(args.pxe_src))
    check_file('{}/libutil.c32'.format(args.pxe_src))

    cmd = None
    if args.dnsmasq_exe:
        cmd = args.dnsmasq_exe
    else:
        cmd = shutil.which('dnsmasq')

    if cmd is None:
        print("Cannot locate 'dnsmasq' executable, please specify with '--dnsmasq-exe'")
        sys.exit(1)

    dnsmasq_args = "-p 0 -h -d -8 - -n --dhcp-boot=pxelinux.0 --pxe-prompt=Booting,0 --enable-tftp --log-debug".split()

    dnsmasq_args.append('--dhcp-range={}.100,static'.format(args.dhcp_net))

    if args.iface:
        dnsmasq_args.append('--interface={}'.format(args.iface))

    with tempfile.TemporaryDirectory() as bootd:
        # Copy pxe files in?
        tftp_root = '{}/tftp'.format(bootd)
        tftp_target = '{}/{}'.format(tftp_root, args.tftp_target)
        pxe_root = '{}/pxelinux.cfg'.format(tftp_root)
        os.makedirs(pxe_root, exist_ok=True)
        os.mkdir(tftp_target)

        shutil.copyfile('{}/pxelinux.0'.format(args.pxe_src), '{}/pxelinux.0'.format(tftp_root))
        shutil.copyfile('{}/menu.c32'.format(args.pxe_src), '{}/menu.c32'.format(tftp_root))
        shutil.copyfile('{}/ldlinux.c32'.format(args.pxe_src), '{}/ldlinux.c32'.format(tftp_root))
        shutil.copyfile('{}/libutil.c32'.format(args.pxe_src), '{}/libutil.c32'.format(tftp_root))

        dnsmasq_args.append('--dhcp-leasefile={}/leases'.format(bootd))
        dnsmasq_args.append('--tftp-root={}'.format(tftp_root))
        count = 0
        for entry in args.host:
            if entry.count(SEP) < 1 or entry.count(SEP) > 3:
                print("'{}' is not a well formed host description, mac address and kernel are required".format(entry))
                sys.exit(1)
            mac,kern,*rest = entry.split(SEP)
            shutil.copyfile(kern, '{}/{}'.format(tftp_target, os.path.basename(kern)))

            cmdline = None
            initrd = None
            if len(rest) >= 1:
                cmdline = rest[0]
            if len(rest) >= 2:
                initrd = rest[1]
                shutil.copyfile(initrd, '{}/{}'.format(tftp_target, os.path.basename(initrd)))

            dnsmasq_args.append('--dhcp-host={},host{},{}.{}'.format(mac, str(count), args.dhcp_net, str(100 + count)))
            count += 1

            pxeconf = """default menu.c32
prompt 0
timeout 1

label {target}
menu default
kernel {target}/{kern}

{append} {init} {cmd}""".format(target=args.tftp_target, kern=os.path.basename(kern),
                    append='append' if initrd is not None and cmdline is not None else '',
                    init='initrd={}/{}'.format(args.tftp_target, os.path.basename(initrd)) if initrd is not None else '',
                    cmd=cmdline if cmdline is not None else '')

            pxe_conf_file = '{}/01-{}'.format(pxe_root, mac.replace(':','-'))
            with open(pxe_conf_file, mode='w') as fd:
                print(pxeconf, file=fd)

        dnsmasq = subprocess.Popen([cmd] + dnsmasq_args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        print(dnsmasq.stdout.read())


if __name__ == '__main__':
    main()

