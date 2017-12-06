##
# Image for pygplates Jupyter server
#

FROM ubuntu:16.04

RUN apt-get update

# install dependencies for pygplates
RUN apt-get install -y libglew-dev
RUN apt-get install -y python2.7-dev
RUN apt-get install -y libboost-dev libboost-python-dev libboost-thread-dev libboost-program-options-dev libboost-test-dev libboost-system-dev
RUN apt-get install -y libqt4-dev
RUN apt-get install -y libgdal1-dev
RUN apt-get install -y libcgal-dev
RUN apt-get install -y libproj-dev
RUN apt-get install -y libqwt-dev
RUN apt-get install -y libxrender-dev libice-dev libsm-dev libfreetype6-dev libfontconfig1-dev

# install wget to enable the pygplates source code to be downloaded from sourceforge
RUN apt-get update
RUN apt-get install -y wget git

# use wget to get the correct pygplates package from sourceforge
RUN wget http://sourceforge.net/projects/gplates/files/pygplates/beta-revision-12/pygplates-ubuntu-trusty_1.5_1_amd64.deb

# use dpkg to install 
RUN dpkg -i pygplates-ubuntu-trusty_1.5_1_amd64.deb


#### Python dependencies
# install all the python and ipython notebook requirements
RUN apt-get install -y gcc python-pip
RUN apt-get install -y python-numpy python-scipy python-matplotlib python-pandas python-sympy python-nose
RUN pip install --upgrade --no-use-wheel pip
RUN pip install --upgrade setuptools
RUN pip install jupyter
RUN pip install ipyparallel
RUN apt-get install -y python-mpltoolkits.basemap
RUN pip install -U scikit-learn
RUN pip install netcdf4
RUN pip install --upgrade pandas
#RUN pip install pyshtools


# Set python path to find pygplates and nlopt

RUN git clone https://github.com/tonysyu/mpltools.git /mpltools
RUN cd /mpltools && python setup.py install

ENV PYTHONPATH ${PYTHONPATH}:/usr/lib:/usr/lib/pygplates/revision12/


### Install GMT
###[Thanks to https://gist.github.com/karronoli]
RUN apt-get install -y ghostscript build-essential cmake libnetcdf-dev libfftw3-dev libpcre3-dev
RUN apt-get install -y gdal-bin
# Note that, due to changing version numbers in the GMT 'top level' ftp with time, it is more
# safe to specify a specific version in the legacy folder where we know the version number
RUN wget ftp://ftp.soest.hawaii.edu/gmt/legacy/gmt-5.3.1-src.tar.xz && \
    tar xf gmt-5.3.1-src.tar.xz
RUN rm -fr ./gmt-5.3.1/build && mkdir ./gmt-5.3.1/build
RUN cd ./gmt-5.3.1/build && \
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


ENV TINI_VERSION v0.8.4
ADD https://github.com/krallin/tini/releases/download/${TINI_VERSION}/tini /usr/local/bin/tini
RUN chmod +x /usr/local/bin/tini

RUN pip install requests

RUN mkdir /workspace
RUN mkdir /workspace/volume
#RUN mkdir /workspace/MODELS


COPY run.sh /build/
RUN chmod +x /build/run.sh

# setup space for working in
VOLUME /workspace/volume
#VOLUME /workspace/MODELS


# launch notebook
WORKDIR /workspace
EXPOSE 8888
ENTRYPOINT ["/usr/local/bin/tini", "--"]

CMD /build/run.sh
