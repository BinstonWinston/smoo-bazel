#!/bin/bash

docker-compose down # Stop any running server

docker-compose build && \
docker-compose up -d && \
docker attach `sudo docker-compose ps -q` --sig-proxy=false