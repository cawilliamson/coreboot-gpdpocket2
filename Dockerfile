FROM ubuntu:18.04

ENV DEBIAN_FRONTEND noninteractive

RUN apt update
RUN apt -y dist-upgrade
RUN apt -y install bison build-essential curl flex git gnat-5 libncurses5-dev m4 nano vim zlib1g-dev 

WORKDIR /usr/src
