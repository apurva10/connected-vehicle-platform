FROM ubuntu:24.04

RUN apt-get update && apt-get install -y build-essential cmake ninja-build git && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
COPY . /workspace

RUN cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build -j2

CMD ["/workspace/build/cvp_app"]
