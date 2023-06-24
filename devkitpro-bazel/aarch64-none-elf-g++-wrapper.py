import subprocess
import sys
from typing import List

CC = '/opt/devkitpro/devkitA64/bin/aarch64-none-elf-g++'

CC_FLAGS = [
    '-gdwarf-2',
    '-gstrict-dwarf',
    '-D__SWITCH__',
    '-DSWITCH',
    '-DNNSDK',
    '-DSMOVER=100',
    '-std=gnu++20',
    '-march=armv8-a+crc+crypto',
    '-mtune=cortex-a57',
    '-mtp=soft',
    '-fPIE',
    '-ftls-model=local-exec',
    '-g',
    '-Wall',
    '-O3',
    '-ffunction-sections',
    '-fno-rtti',
    '-fno-exceptions',
    '-Wno-invalid-offsetof',
    '-Wno-volatile',
    '-fno-rtti',
    '-fomit-frame-pointer',
    '-fno-exceptions',
    '-fno-asynchronous-unwind-tables',
    '-fno-unwind-tables',
]

LINKER_FLAGS = [
    '-specs=/specs/switch.specs',
    '-g',
    '-march=armv8-a+crc+crypto',
    '-mtune=cortex-a57',
    '-mtp=soft',
    '-fPIE',
    '-ftls-model=local-exec',
    '-Wl,-Map,/patches/maps/100/main.map',
    '-Wl,--version-script=/patches/exported.txt',
    '-Wl,-init=__custom_init',
    '-Wl,-fini=__custom_fini',
    '-nostdlib',
]

def main(args: List[str]):
    print(f'g++ ARGS: {args}')
    for arg in args:
        if arg.endswith('.so'):
            print(f'Skipping .so output of {arg}')
            with open(arg, 'w+') as f:
                f.write('empty\n')

    subprocess.run(['ls external'])
    subprocess.run([CC] + CC_FLAGS + LINKER_FLAGS + args)

if __name__ == '__main__':
    main(sys.argv[1:])