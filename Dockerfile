FROM cromo/devkitarm:r46

# --------------------------------------------------------------
# Python 3.3 - Install from Source
# Note: below pulled from python/3.3 and more or less unmodified
# --------------------------------------------------------------

# Install tools we'll use for the python build
RUN apt-get update && \
  apt-get -y install \
  build-essential=11.7 \
  cmake=3.0.2-1+deb8u1 \
  curl=7.38.0-4+deb8u16 \
  git=1:2.1.4-2.1+deb8u8 \
  golang=2:1.3.3-1+deb8u2 \
  libfreetype6=2.5.2-3+deb8u4 \
  libjpeg-dev=1:1.3.1-12+deb8u2 \
  libssl-dev=1.0.1t-1+deb8u12  \
  libxi6=2:1.7.4-1+deb8u1 \
  libxxf86vm1=1:1.1.3-1+b1 \
  python3=3.4.2-2 \
  python3-pip=1.5.6-5 \
  zlib1g-dev=1:1.2.8.dfsg-2+deb8u1 && \
  rm -rf /var/lib/apt/lists/*

# Install the libraries that the dsgx-converter relies on
RUN pip3 install docopt==0.6.2
RUN pip3 install euclid3
RUN pip3 install Pillow==4.3.0

# Install Blender, so that it can perform object exports
# directly as part of the build
RUN curl -O https://download.blender.org/release/Blender2.74/blender-2.74-linux-glibc211-x86_64.tar.bz2 && \
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
