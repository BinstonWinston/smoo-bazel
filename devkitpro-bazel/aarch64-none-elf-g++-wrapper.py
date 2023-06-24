import argparse
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

def parse_args(args: List[str]):
    parser = argparse.ArgumentParser(
                    prog='ProgramName',
                    description='What the program does',
                    epilog='Text at the bottom of help')
    parser.add_argument('-o', '--output')
    return parser.parse_known_intermixed_args(args)[0]

def main(args: List[str]):
    print(f'g++ ARGS: {args}')
    parsed_args = parse_args(args)
    if parsed_args.output.endswith('.so'):
        print(f'Skipping .so output of {parsed_args.output}')
        with open(parsed_args.output, 'w+') as f:
            f.write('empty\n')
            return

    subprocess.run([CC] + CC_FLAGS + LINKER_FLAGS + args)

if __name__ == '__main__':
    main(sys.argv[1:])