#!/usr/bin/env bash

docker build -t coreboot .
docker run --rm -it -v $(pwd):/data coreboot bash
