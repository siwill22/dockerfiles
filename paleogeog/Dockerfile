##
# Image for pygplates Jupyter server
#

FROM ubuntu:16.04

RUN apt-get update
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
RUN wget http://sourceforge.net/projects/gplates/files/pygplates/beta-revision-12/pygplates-ubuntu-xenial_1.5_1_amd64.deb

# use dpkg to install 
RUN dpkg -i pygplates-ubuntu-xenial_1.5_1_amd64.deb


#### Python dependencies
# install all the python and ipython notebook requirements
RUN apt-get install -y gcc python-pip
RUN apt-get install -y python-numpy python-scipy python-matplotlib python-pandas python-sympy python-nose
RUN pip install --upgrade --no-use-wheel pip
RUN pip install --upgrade setuptools

RUN apt-get install -y libssl-dev

RUN pip install jupyterlab
RUN pip install ipyparallel
RUN apt-get install -y python-mpltoolkits.basemap
RUN pip install -U scikit-learn
RUN pip install netcdf4
RUN pip install --upgrade pandas
RUN pip install healpy
RUN pip install moviepy
RUN pip install xarray
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
RUN rm -fr /gmt-5.3.1/build && mkdir /gmt-5.3.1/build
RUN wget ftp://ftp.soest.hawaii.edu/gmt/legacy/gshhg-gmt-2.3.5.tar.gz && \
    mkdir /gmt-5.3.1/coast && \
    mv gshhg-gmt-2.3.5.tar.gz /gmt-5.3.1/coast/gshhg-gmt-2.3.5.tar.gz 
RUN cd /gmt-5.3.1/coast && tar xf gshhg-gmt-2.3.5.tar.gz
RUN wget ftp://ftp.soest.hawaii.edu/gmt/legacy/dcw-gmt-1.1.1.tar.gz && \
    mkdir /gmt-5.3.1/dcw && \
    mv dcw-gmt-1.1.1.tar.gz /gmt-5.3.1/dcw/dcw-gmt-1.1.1.tar.gz
RUN cd /gmt-5.3.1/dcw && tar xf dcw-gmt-1.1.1.tar.gz
RUN cd /gmt-5.3.1/build && \
    cmake -DCMAKE_INSTALL_PREFIX=/usr \
      -DGSHHG_PATH=/gmt-5.3.1/coast/gshhg-gmt-2.3.5 \
      -DGMT_LIBDIR=lib \
      -DDCW_ROOT=/gmt-5.3.1/dcw/dcw-gmt-1.1.1.tar \
      -DGMT_DATADIR=share/gmt \
      -DGMT_MANDIR=share/man \
      -DGMT_DOCDIR=share/doc/gmt \
      -DCMAKE_BUILD_TYPE=Release .. && \
    make && make install


ENV TINI_VERSION v0.8.4
ADD https://github.com/krallin/tini/releases/download/${TINI_VERSION}/tini /usr/local/bin/tini
RUN chmod +x /usr/local/bin/tini

RUN pip install requests


RUN git clone https://github.com/siwill22/pigplates.git /pgp/pigplates
RUN git clone https://github.com/siwill22/gwstools.git /pgp/gwstools

ENV PYTHONPATH ${PYTHONPATH}:/pgp/pigplates:/pgp/gwstools

#ADD ~/.ssh/github.sydney.edu.au_rsa.pub /tmp/
#RUN ssh-agent /tmp
#RUN git clone https://github.sydney.edu.au/swil4597/paleotopography.git /pgp/paleotopography
#RUN rm /tmp/github.sydney.edu.au_rsa.pub


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
