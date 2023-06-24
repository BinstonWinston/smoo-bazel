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
    generate_map_file = '--special-dkp-output-map-file-instead-of-shared-lib' in args
    
    output_so = None
    if generate_map_file:
        output_so = [arg for arg in args if arg.endswith('.so')][0]
        # Remove output arg and special arg
        args = [arg for arg in args if not arg.endswith('.so') and arg != '-o' and arg != '--special-dkp-output-map-file-instead-of-shared-lib']
        # Add output arg back in at some ignored filepath (will be deleted by bazel sandboxing)
        args += ['-o', f'{output_so}.ignore']
        # Add map output at original .so filepath so it isn't clobbered by bazel sandboxing
        args += ['-Xlinker', f'-Map={output_so}']

    print(args)

    subprocess.run([CC] + CC_FLAGS + LINKER_FLAGS + args)

    # if generate_map_file is not None:
    #     with open(output_so, 'r') as f:
    #         print("MYMAP")
    #         print(f.read())
    # else:
    #     print("No map output")

if __name__ == '__main__':
    main(sys.argv[1:])