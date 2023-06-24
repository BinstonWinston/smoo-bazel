#!/bin/bash

dazel build \
    --incompatible_enable_cc_toolchain_resolution \
    --platforms=@devkitpro//:devkitpro_nx_platform \
    --action_env=DEVKITPRO=/opt/devkitpro \
    \
    --toolchain_resolution_debug \
    --verbose_failures \
    --sandbox_debug \
    --experimental_ui_max_stdouterr_bytes=10048576 \
    //proto:test_packet_cc_proto