#!/bin/bash

dazel build \
    --incompatible_enable_cc_toolchain_resolution \
    --platforms=//:devkitpro_nx_platform \
    --toolchain_resolution_debug \
    //src:hello-world