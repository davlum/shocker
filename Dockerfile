FROM ubuntu:20.10

RUN apt-get update && \
  apt-get install -y \
  gcc \
  && apt-get clean

WORKDIR /root

