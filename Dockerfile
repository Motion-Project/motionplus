FROM docker.io/ubuntu:20.04
WORKDIR /tmp/motionplus
COPY . .

RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        autoconf automake autopoint pkgconf libtool libjpeg8-dev build-essential libzip-dev gettext libmicrohttpd-dev libavformat-dev libavcodec-dev libavutil-dev libswscale-dev libavdevice-dev git-core ffmpeg && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/ && \
    autoreconf -fiv && \
    ./configure && \
    make && \
    make install && \
    cd .. && \
    rm -rf /tmp/motionplus

WORKDIR /usr/local/bin/

ENTRYPOINT [ "/usr/local/bin/motionplus" ]
CMD [ "-n" ]
