##
# Image for pygplates Jupyter server
#

FROM ubuntu:16.04

RUN apt-get update

### Install GMT
###[Thanks to https://gist.github.com/karronoli]
RUN apt-get install -y ghostscript 
RUN apt-get install -y build-essential 
RUN apt-get install -y cmake 
RUN apt-get install -y libnetcdf-dev 
RUN apt-get install -y libfftw3-dev 
RUN apt-get install -y libpcre3-dev
RUN apt-get install -y libgdal1-dev
RUN apt-get install -y liblapack-dev
RUN apt-get install -y libcurl4-gnutls-dev

RUN apt-get install -y subversion
RUN apt-get install -y wget

RUN svn checkout svn://gmtserver.soest.hawaii.edu/gmt5/trunk ./gmt5-dev

# Note that, due to changing version numbers in the GMT 'top level' ftp with time, it is more
# safe to specify a specific version in the legacy folder where we know the version number
#RUN wget ftp://ftp.soest.hawaii.edu/gmt/gmt-5.4.1-src.tar.xz && \
#    tar xf gmt-5.4.1-src.tar.xz
#RUN rm -fr ./gmt-5.4.1/build && mkdir ./gmt-5.4.1/build

RUN wget ftp://ftp.soest.hawaii.edu/gmt/legacy/gshhg-gmt-2.3.5.tar.gz && \
    mkdir /gmt5-dev/coast && \
    mv gshhg-gmt-2.3.5.tar.gz /gmt5-dev/coast/gshhg-gmt-2.3.5.tar.gz 
RUN cd /gmt5-dev/coast && tar xf gshhg-gmt-2.3.5.tar.gz
RUN wget ftp://ftp.soest.hawaii.edu/gmt/legacy/dcw-gmt-1.1.1.tar.gz && \
    mkdir /gmt5-dev/dcw && \
    mv dcw-gmt-1.1.1.tar.gz /gmt5-dev/dcw/dcw-gmt-1.1.1.tar.gz
RUN cd /gmt5-dev/dcw && tar xf dcw-gmt-1.1.1.tar.gz

RUN mkdir ./gmt5-dev/build
RUN cd ./gmt5-dev/build && \
    cmake -DCMAKE_INSTALL_PREFIX=/usr \
      -DGSHHG_ROOT=/usr/share/gmt/coast \
      -DGMT_LIBDIR=lib \
      -DDCW_ROOT=/usr/share/gmt/dcw \
      -DGMT_DATADIR=share/gmt \
      -DGMT_MANDIR=share/man \
      -DGMT_DOCDIR=share/doc/gmt \
      -DCMAKE_BUILD_TYPE=Release .. && \
    make && make install
#RUN apt-get install gmt-dcw gmt-gshhg

#RUN apt-get install -y ffmpeg
#RUN apt-get install -y graphicsmagick

ENV TINI_VERSION v0.8.4
ADD https://github.com/krallin/tini/releases/download/${TINI_VERSION}/tini /usr/local/bin/tini
RUN chmod +x /usr/local/bin/tini


RUN mkdir /workspace
RUN mkdir /workspace/volume

# setup space for working in
VOLUME /workspace/volume

# launch notebook
WORKDIR /workspace
#EXPOSE 8888
ENTRYPOINT ["/usr/local/bin/tini", "--"]

#CMD ["more /gmt5-dev/README"]
CMD ["bash"]

