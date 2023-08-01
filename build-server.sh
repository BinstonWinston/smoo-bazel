#!/bin/bash

dazel build \
    --verbose_failures \
    --sandbox_debug \
    --experimental_ui_max_stdouterr_bytes=10048576 \
    //server/Server:smoo_server