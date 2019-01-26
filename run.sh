#!/usr/bin/env bash

docker build -t coreboot .
docker run --rm -it -v $(pwd):/usr/src coreboot bash
