FROM ubuntu:20.04
RUN apt-get update && apt-get install -yq tzdata && ln -fs /usr/share/zoneinfo/America/New_York /etc/localtime && dpkg-reconfigure -f noninteractive tzdata

RUN apt-get update && apt-get install -y g++ make postgresql libpq-dev libssl-dev libxerces-c-dev libpqxx-dev

RUN mkdir -p /code
ADD . /code/
WORKDIR /code

RUN chmod o+x testserver.sh
