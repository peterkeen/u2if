FROM python:slim

WORKDIR /app
RUN mkdir /app/firmware

RUN apt-get update && apt-get install -y --no-install-recommends libusb-1.0 libudev-dev libhidapi-hidraw0 build-essential libusb-1.0.0-dev cmake git && apt-get clean
RUN pip install hidapi pyserial
ENV PICO_SDK=/app/pico-sdk
RUN git clone https://github.com/raspberrypi/pico-sdk.git && git clone https://github.com/raspberrypi/picotool.git && cd picotool && cmake && make install && cd /app && rm -rf pico-sdk picotool

COPY *.uf2 /app/firmware
COPY *.tar.gz /app

RUN pip install /app/u2if-*.tar.gz
