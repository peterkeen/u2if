FROM python:latest

WORKDIR /app
RUN mkdir /app/firmware

COPY *.uf2 /app/firmware
COPY *.tar.gz /app

RUN pip install /app/u2if-*.tar.gz