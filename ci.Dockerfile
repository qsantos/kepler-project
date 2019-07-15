FROM alpine:3.10.1

RUN apk add gcc cppcheck make libc-dev
RUN wget "https://github.com/DaveGamble/cJSON/archive/v1.7.12.tar.gz" \
    && tar xvf "v1.7.12.tar.gz" \
    && (cd "cJSON-1.7.12"; make; make install PREFIX=/usr) \
    && rm -Rf "cJSON-1.7.12" "v1.7.12.tar.gz"
