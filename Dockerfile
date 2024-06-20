FROM python:slim

WORKDIR /app
RUN mkdir /app/firmware

RUN apt-get update && apt-get install -y --no-install-recommends libusb-1.0 libudev-dev libhidapi-hidraw0 && apt-get clean
RUN pip install hidapi pyserial

COPY *.uf2 /app/firmware
COPY *.tar.gz /app
COPY picotool /app

RUN pip install /app/u2if-*.tar.gz
