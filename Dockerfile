FROM ubuntu:18.04

RUN apt-get update && apt-get install -y build-essential cmake valgrind git

LABEL com.circleci.preserve-entrypoint=true

ENTRYPOINT contacts

