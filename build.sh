#!/bin/bash

dazel build \
    --incompatible_enable_cc_toolchain_resolution \
    --platforms=//devkitpro-bazel:devkitpro_nx_platform \
    --toolchain_resolution_debug \
    --verbose_failures \
    --sandbox_debug \
    //src:hello-world-nro