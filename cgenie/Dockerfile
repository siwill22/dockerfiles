##
# Image for genie
#

FROM ubuntu:16.04

RUN apt-get update

RUN apt-get install -y gfortran
RUN apt-get install -y build-essential

RUN apt-get install -y subversion
RUN apt-get install -y wget

RUN apt-get install -y automake autoconf libtool
RUN apt-get install -y xsltproc

RUN apt-get install -y bc
RUN apt-get install -y python

RUN mkdir -p /netcdf
ADD netcdf-4.0 /netcdf
RUN cd /netcdf && ./configure && make && make install


# This may not be needed
ENV TINI_VERSION v0.8.4
ADD https://github.com/krallin/tini/releases/download/${TINI_VERSION}/tini /usr/local/bin/tini
RUN chmod +x /usr/local/bin/tini

CMD ["bash"]
