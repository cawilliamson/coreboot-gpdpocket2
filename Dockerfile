FROM ubuntu:18.04

ENV DEBIAN_FRONTEND noninteractive

RUN apt update
RUN apt -y dist-upgrade
RUN apt -y install bison build-essential curl flex git gnat-5 libncurses5-dev m4 zlib1g-dev 

RUN git clone --depth=1 https://review.coreboot.org/coreboot.git /usr/src/coreboot
