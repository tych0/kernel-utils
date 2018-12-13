#!/usr/bin/env python3
# requires >= 3.5, assumes . is a kernel tree

import collections
import subprocess

Arch = collections.namedtuple('Arch', ['kname', 'tuple'])

arches = [
    Arch('alpha', 'alpha-linux-gnu'),
    Arch('x86', ''),
    Arch('s390', 's390x-linux-gnu'),
    Arch('arm', 'arm-none-eabi'),
    Arch('arm64', 'aarch64-linux-gnu'),
    Arch('h8300', 'h8300-hms'),
    Arch('x86', 'i686-linux-gnu'),
    Arch('m68k', 'm68k-linux-gnu'),
    Arch('mips', 'mips-linux-gnu'),
    Arch('powerpc', 'powerpc64-linux-gnu'),
    Arch('riscv', 'riscv64-linux-gnu'),
    Arch('sh', 'sh4-linux-gnu'),
    Arch('sparc', 'sparc64-linux-gnu'),
]

def install_compilers():
    compiler_packages = ["gcc-"+a.tuple for a in arches if a.tuple != ""]
    cmd = ["sudo", "apt", "install", "-yy"] + compiler_packages
    subprocess.run(cmd, stdin=subprocess.DEVNULL, check=True)

def do_arch_build(arch):
    def run_make_arch(command):
        cmd = [
            "make",
            "ARCH=%s" % arch.kname,
        ]
        if arch.tuple != "":
            cmd += ["CROSS_COMPILE=%s-" % arch.tuple]
        cmd += [command]
        subprocess.run(cmd, stdin=subprocess.DEVNULL, check=True)
    run_make_arch("allyesconfig")
    run_make_arch("-j40")

install_compilers()
for arch in arches:
    do_arch_build(arch)
