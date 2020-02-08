FROM cromo/devkitarm:r46

# --------------------------------------------------------------
# Python 3.3 - Install from Source
# Note: below pulled from python/3.3 and more or less unmodified
# --------------------------------------------------------------

# Install tools we'll use for the python build
RUN apt-get update && \
  apt-get -y install \
  build-essential \
  cmake \
  curl \
  git \
  golang \
  libfreetype6 \
  libjpeg-dev \
  libssl-dev  \
  libxi6 \
  libxxf86vm1 \
  python3 \
  python3-pip \
  zlib1g-dev && \
  rm -rf /var/lib/apt/lists/*

# Install the libraries that the dsgx-converter relies on
RUN pip3 install docopt
RUN pip3 install euclid3
RUN pip3 install Pillow==4.3.0

# Install Blender, so that it can perform object exports
# directly as part of the build
RUN curl -O http://download.blender.org/release/Blender2.74/blender-2.74-linux-glibc211-x86_64.tar.bz2 && \
  tar xf blender-2.74-linux-glibc211-x86_64.tar.bz2 && \
  mv blender-2.74-linux-glibc211-x86_64 /opt/ && \
  rm blender-2.74-linux-glibc211-x86_64.tar.bz2 && \
  echo '/opt/blender-2.74-linux-glibc211-x86_64/lib' > /etc/ld.so.conf.d/blender.conf && \
  ldconfig

RUN mkdir -p /opt/gopath/bin
ENV GOPATH /opt/gopath
ENV PATH $PATH:/opt/blender-2.74-linux-glibc211-x86_64:/opt/gopath/bin

# Clone in the dsgx-converter project
RUN mkdir /opt/dsgx-converter \
  && git clone https://github.com/zeta0134/dsgx-converter.git /opt/dsgx-converter \
  && cd /opt/dsgx-converter \
  && git reset --hard fd40a15f8035038e42c7f0329d06c583c24354e3 \
  && chmod +x /opt/dsgx-converter/model2dsgx.py \
  && ln -s /opt/dsgx-converter/model2dsgx.py /usr/local/bin/model2dsgx

# Clone in the dtex texture converter

RUN mkdir -p /opt/gopath/src/dtex \
  && git clone https://github.com/cromo/dtex.git /opt/gopath/src/dtex \
  && cd /opt/gopath/src/dtex \
  && git reset --hard da3b6c0e877e05411d9d43973bfd665c2dcdafe7 \
  && go get .

ARG external_username
ARG external_uid
ARG external_gid
RUN mkdir /home/$external_username && \
  groupadd --gid $external_gid $external_username && \
  useradd --uid $external_uid --gid $external_gid $external_username && \
  chown $external_username /home/$external_username /source && \
  chgrp $external_username /home/$external_username /source
USER $external_username

CMD make
