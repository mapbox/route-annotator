FROM ubuntu:16.04

RUN apt-get -y update
RUN apt-get -y install g++-5 cmake git libstdc++-5-dev curl \
                       libboost-dev libboost-test1.58-dev \
                       libbz2-dev libexpat-dev libbz2-dev \
                       pkg-config libstxxl-dev zlib1g-dev

WORKDIR /opt
RUN git clone --depth 1 --branch v0.31.0 https://github.com/creationix/nvm.git
RUN /bin/bash -c "source /opt/nvm/nvm.sh && nvm install v4"

RUN useradd -ms /bin/bash mapbox
USER mapbox
ENV HOME /home/mapbox

WORKDIR /home/mapbox
RUN echo "source /opt/nvm/nvm.sh" > .bashrc
RUN echo "source /home/mapbox/.bashrc" > .profile

EXPOSE 5000
