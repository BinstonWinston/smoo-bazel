import argparse
import subprocess
import sys
from typing import List

CC = '/opt/devkitpro/devkitA64/bin/aarch64-none-elf-g++'

ARCH_FLAGS = [
    '-march=armv8-a',
    '-mtune=cortex-a57',
    '-mtp=soft',
    '-fPIC',
    '-ftls-model=local-exec',
]

C_FLAGS = [
    '-g',
    '-Wall',
    '-ffunction-sections',
] + ARCH_FLAGS + [
    '-D__SWITCH__',
    '-DSMOVER=100',
    '-O3',
    '-DNNSDK',
    '-DSWITCH',
    # '-DBUILDVERSTR=$(BUILDVERSTR)',
    # '-DBUILDVER=$(BUILDVER)',
    # '-DDEBUGLOG=$(DEBUGLOG)',
    # '-DSERVERIP=$(SERVERIP)',
]

CC_FLAGS = [
    # '-gdwarf-2',
    # '-gstrict-dwarf',
] + C_FLAGS + [
    '-Wno-invalid-offsetof',
    '-Wno-volatile', 
    '-fno-rtti',
    '-fomit-frame-pointer',
    '-fno-exceptions',
    '-fno-asynchronous-unwind-tables',
    '-fno-unwind-tables',
    '-std=gnu++20',
]

AS_FLAGS = ['-g'] + ARCH_FLAGS

LINKER_FLAGS = [
    '-specs=/specs/switch.specs',
    '-g',
] + ARCH_FLAGS + [
    # '-Wl,-Map,/patches/maps/100/main.map',
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
    parser.add_argument('--special-link-opt-generate-linker-map', dest='generate_linker_map', action='store_true')
    parser.add_argument('--special-link-opt-whole-archive', dest='whole_archive', action='store_true')
    return parser.parse_known_intermixed_args(args)

def main(args: List[str]):
    parsed_args, unparsed_args = parse_args(args)
    if parsed_args.whole_archive:
        args = unparsed_args
        args = [arg for arg in args if arg.endswith('.a')] # Strip cc_shared_library args besides .a static libs
        # Needed because cc_shared_library appends user_link_flags to the end, meaning -Wl,--whole-archive won't be applied to our input static lib deps
        args = ['-Wl,--whole-archive'] + args # Prepend flag so it gets correctly applied to linked libraries
        args += ['-o', parsed_args.output] # Append output arg since it is removed from unparsed_args during arg parsing
    if parsed_args.generate_linker_map:
        args = unparsed_args
        # Redirect linked library output to a new name that will be deleted by bazel sandboxing
        args += ['-o', f'{parsed_args.output}.ignore']
        # Generate linker map in place of original library output path so it won't be clobbered by bazel sandboxing
        args += ['-Xlinker', f'-Map={parsed_args.output}']

    if [arg for arg in args if arg.endswith('.s')] != []: # Assembly file
        args = AS_FLAGS + args
    elif parsed_args.output.endswith('.o'): # Any other input source file .c/.cc/.cpp with output .o object file
        args = CC_FLAGS + args
    else:
        args = LINKER_FLAGS + args 

    print(f"g++ ARGS MODIFIED {args}")
    subprocess.run([CC] + args)

if __name__ == '__main__':
    main(sys.argv[1:])