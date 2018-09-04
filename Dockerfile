FROM ubuntu:18.04

RUN apt-get update && apt-get install -y build-essential cmake valgrind git libssl-dev doxygen
