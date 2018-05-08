##
# Image for pygplates Jupyter server
#

FROM siwill22/pygplates16

#### Python dependencies
# install all the python and ipython notebook requirements
RUN apt-get install -y gcc python-pip
RUN pip install --upgrade --no-use-wheel pip
RUN pip install --upgrade setuptools
RUN pip install --ignore-installed jupyter
RUN pip install jupyterlab
#RUN pip install ipyparallel
RUN apt-get install -y python-numpy python-scipy python-matplotlib python-pandas python-sympy python-nose
RUN apt-get install -y python-mpltoolkits.basemap
RUN pip install -U scikit-learn
RUN pip install netcdf4
RUN pip install --upgrade pandas
RUN pip install healpy
RUN pip install moviepy
RUN pip install xarray
#RUN pip install pyshtools

# NB python path to find pygplates and nlopt should already be set in pygplates16 image

RUN git clone https://github.com/tonysyu/mpltools.git /mpltools
RUN cd /mpltools && python setup.py install



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
RUN pip install geojson
RUN pip install joblib
RUN apt-get install -y gfortran
RUN pip install litho1pt0

RUN mkdir /workspace

COPY run.sh /build/
RUN chmod +x /build/run.sh

RUN git clone https://github.com/GPlates/pygplates-tutorials.git /pygplates-tutorials
RUN git clone https://github.com/siwill22/gwstools.git /gwstools
RUN git clone https://github.com/siwill22/gws-examples.git /gws-examples


RUN mkdir /workspace/tutorials/
RUN cp -av /pygplates-tutorials/* /workspace/tutorials/

RUN mkdir /workspace/web-service-examples
RUN cp -av /gws-examples/python/* /workspace/web-service-examples

ENV PYTHONPATH ${PYTHONPATH}:/gwstools

# setup space for working in
VOLUME /workspace/volume

# launch notebook
WORKDIR /workspace
EXPOSE 8888
ENTRYPOINT ["/usr/local/bin/tini", "--"]

CMD /build/run.sh
