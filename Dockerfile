FROM python:latest

WORKDIR /app
RUN mkdir /app/firmware

RUN apt-get update && apt-get install libusb-1.0 libudev-dev libhidapi-libusb0
RUN pip install hidapi pyserial

COPY *.uf2 /app/firmware
COPY *.tar.gz /app

RUN pip install /app/u2if-*.tar.gz
