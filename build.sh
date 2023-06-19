#!/bin/bash

dazel build \
    --incompatible_enable_cc_toolchain_resolution \
    --platforms=@devkitpro//:devkitpro_nx_platform \
    --action_env=DEVKITPRO=/opt/devkitpro \
    \
    --toolchain_resolution_debug \
    --verbose_failures \
    --sandbox_debug \
    //src:smoo_nro