FROM gcc

RUN echo "deb http://ftp.fr.debian.org/debian/ buster main">/etc/apt/sources.list \
    && apt-get update \
    && apt-get install -y --no-install-recommends libcjson-dev \
    && rm -rf /var/lib/apt/lists/*
