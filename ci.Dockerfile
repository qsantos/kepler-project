FROM alpine:3.10.1

RUN apk --no-cache add gcc cppcheck make libc-dev && \
    rm /usr/libexec/gcc/x86_64-alpine-linux-musl/*/cc1obj && \
    rm /usr/libexec/gcc/x86_64-alpine-linux-musl/*/lto1 && \
    rm /usr/libexec/gcc/x86_64-alpine-linux-musl/*/lto-wrapper
RUN wget "https://github.com/DaveGamble/cJSON/archive/v1.7.12.tar.gz" \
    && tar xvf "v1.7.12.tar.gz" \
    && (cd "cJSON-1.7.12"; make; make install PREFIX=/usr) \
    && rm -Rf "cJSON-1.7.12" "v1.7.12.tar.gz"
