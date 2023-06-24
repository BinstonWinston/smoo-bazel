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
    generate_map_file = '--special-dkp-output-map-file-instead-of-lib' in args
    
    print(args)

    if generate_map_file:
        output_file = None
        for i in range(len(args)):
            if args[i] == '-o':
                output_file = args[i+1]

        if output_file.endswith('.so') or output_file.endswith('.a'):
            # Remove output arg and special arg
            args = [arg for arg in args if arg not in [output_file, '-o', '--special-dkp-output-map-file-instead-of-lib']]
            # Add output arg back in at some ignored filepath (will be deleted by bazel sandboxing)
            args += ['-o', f'{output_file}.ignore']
            # Add map output at original .so filepath so it isn't clobbered by bazel sandboxing
            args += ['-Xlinker', f'-Map={output_file}']

            print(f'Replacing output binary with linker map: {output_file}')

            print(args)

    subprocess.run([CC] + CC_FLAGS + LINKER_FLAGS + args)

if __name__ == '__main__':
    main(sys.argv[1:])