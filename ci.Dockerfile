FROM debian:buster-slim

WORKDIR /home

RUN echo "deb http://ftp.fr.debian.org/debian/ buster main">/etc/apt/sources.list \
    && apt-get update \
    && apt-get install -y --no-install-recommends \
        cppcheck git make \
        g++ libglew-dev libglfw3-dev libglm-dev libstb-dev libcjson-dev \
        wget ca-certificates zip unzip cmake mingw-w64 \
    && rm -rf /var/lib/apt/lists/*

ENV INSTALL_DIR="/usr/local/x86_64-w64-mingw32"
ENV CMAKE_CFG="/home/Toolchain-mingw32.cmake"

COPY Toolchain-mingw32.cmake "$CMAKE_CFG"

# cJSON
RUN wget "https://github.com/DaveGamble/cJSON/archive/v1.7.12.tar.gz" -O "cJSON-1.7.12.tar.gz" \
    && tar xvf "cJSON-1.7.12.tar.gz" \
    && rm "cJSON-1.7.12.tar.gz" \
    && cd "cJSON-1.7.12" \
    && cmake -DCMAKE_TOOLCHAIN_FILE="$CMAKE_CFG" -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR/" . \
    && make install \
    && mv "$INSTALL_DIR/lib/libcjson.dll.a" "$INSTALL_DIR/lib/libcjson.a" \
    && cd .. \
    && rm -Rf "cJSON-1.7.12"

# GLM
RUN wget "https://github.com/g-truc/glm/archive/0.9.9.5.tar.gz" -O "glm-0.9.9.5.tar.gz" \
    && tar xvf "glm-0.9.9.5.tar.gz" \
    && rm "glm-0.9.9.5.tar.gz" \
    && cd  "glm-0.9.9.5" \
    && cmake -DCMAKE_TOOLCHAIN_FILE="$CMAKE_CFG" -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" . \
    && make install \
    && cd .. \
    && rm -Rf "glm-0.9.9.5"

# GLEW
RUN wget "https://github.com/nigels-com/glew/releases/download/glew-2.1.0/glew-2.1.0-win32.zip" -O "glew-2.1.0-win32.zip" \
    && unzip "glew-2.1.0-win32.zip" \
    && rm "glew-2.1.0-win32.zip" \
    && cd "glew-2.1.0" \
    && cp -r include/* /usr/local/x86_64-w64-mingw32/include/ \
    && cp lib/Release/x64/* /usr/local/x86_64-w64-mingw32/lib/ \
    && cd .. \
    && rm -Rf "glew-2.1.0"

# GLFW
RUN wget "https://github.com/glfw/glfw/releases/download/3.3/glfw-3.3.bin.WIN64.zip" -O "glfw-3.3.bin.WIN64.zip" \
    && unzip "glfw-3.3.bin.WIN64.zip" \
    && rm "glfw-3.3.bin.WIN64.zip" \
    && cd "glfw-3.3.bin.WIN64" \
    && cp -r include/* "$INSTALL_DIR/include/" \
    && cp lib-mingw-w64/* "$INSTALL_DIR/lib/" \
    && cd .. \
    && rm -Rf "glfw-3.3.bin.WIN64"

# stb_image
RUN mkdir "$INSTALL_DIR/include/stb"
RUN wget "https://raw.githubusercontent.com/nothings/stb/2c2908f50515dcd939f24be261c3ccbcd277bb49/stb_image.h" -O "$INSTALL_DIR/include/stb/stb_image.h"
