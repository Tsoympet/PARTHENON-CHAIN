FROM ubuntu:22.04

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
       build-essential cmake git ca-certificates \
       libssl-dev libboost-all-dev libleveldb-dev python3 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /opt/drachma
COPY . /opt/drachma

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build --parallel

EXPOSE 9333 8332
VOLUME ["/opt/drachma/.drachma"]

ENTRYPOINT ["/opt/drachma/build/layer1-core/drachmad"]
CMD ["--network", "testnet", "--datadir", "/opt/drachma/.drachma"]
