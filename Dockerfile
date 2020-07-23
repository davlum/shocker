FROM ubuntu:20.10

RUN apt-get update && \
  apt-get install -y \
  vim \
  gcc \
  && apt-get clean

WORKDIR /root

