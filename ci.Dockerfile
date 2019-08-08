FROM debian:buster-slim

RUN echo "deb http://ftp.fr.debian.org/debian/ buster main">/etc/apt/sources.list \
    && apt-get update \
    && apt-get install -y --no-install-recommends g++ cppcheck make \
        libglew-dev libglfw3-dev libglm-dev libstb-dev libcjson-dev \
    && rm -rf /var/lib/apt/lists/*
