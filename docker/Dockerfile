FROM ubuntu:16.04

RUN apt-get update -y
RUN apt-get -y install g++-5 cmake git libstdc++-5-dev
RUN apt-get -y install libboost-all-dev libbz2-dev libexpat-dev libbz2-dev pkg-config libstxxl-dev
RUN apt-get -y install curl
#RUN apt-get -y install nodejs nodejs-dev npm
#RUN ln -s /usr/bin/nodejs /usr/bin/node

RUN useradd -ms /bin/bash mapbox
USER mapbox
ENV HOME /home/mapbox
WORKDIR /home/mapbox

RUN git clone --depth 1 --branch v0.31.0 https://github.com/creationix/nvm.git
