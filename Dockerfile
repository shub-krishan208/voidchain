FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=interactive
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    curl \
    unzip \
    zip \
    pkg-config \
    && rm -rf /var/ar/lib/apt/lists/*

WORKDIR /opt
RUN git clone http://github.com/microsoft/vcpkg.git \
    && ./vcpkg/bootstrap-vcpkg.sh


ENV VCPKG_ROOT=/opt/vcpkg
ENV PATH="${VCPKG_ROOT}:${PATH}"

WORKDIR /app
COPY . .

RUN cmake --preset debug . && \
    cmake --build --preset debug
EXPOSE 18169
CMD ["bash", "./run.sh"]

