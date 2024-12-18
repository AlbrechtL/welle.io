# welle-cli Dockerfile

FROM ubuntu:24.04
LABEL maintainer "albrechtloh@gmx.de"
LABEL description "welle-cli Docker image"

RUN apt-get update -y && \
apt-get install -y git build-essential cmake plocate libfaad-dev libmpg123-dev libfftw3-dev librtlsdr-dev libusb-1.0-0-dev mesa-common-dev libglu1-mesa-dev libpulse-dev libsoapysdr-dev libairspy-dev libmp3lame-dev xxd

RUN  git clone https://github.com/AlbrechtL/welle.io.git && \
cd welle.io && \
mkdir build && \
cd build && \
cmake ../ -DBUILD_WELLE_IO=off -Wno-dev && \
make -j 12 && \
make install

EXPOSE 2000
CMD ["/bin/bash"]
