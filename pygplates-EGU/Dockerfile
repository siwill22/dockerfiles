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
RUN wget http://sourceforge.net/projects/gplates/files/pygplates/beta-revision-12/pygplates-ubuntu-xenial_1.5_1_amd64.deb

# use dpkg to install 
RUN dpkg -i pygplates-ubuntu-xenial_1.5_1_amd64.deb


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
RUN pip install healpy
RUN pip install moviepy
RUN pip install xarray
#RUN pip install pyshtools


# Set python path to find pygplates and nlopt

RUN git clone https://github.com/tonysyu/mpltools.git /mpltools
RUN cd /mpltools && python setup.py install

ENV PYTHONPATH ${PYTHONPATH}:/usr/lib:/usr/lib/pygplates/revision12/



ENV TINI_VERSION v0.8.4
ADD https://github.com/krallin/tini/releases/download/${TINI_VERSION}/tini /usr/local/bin/tini
RUN chmod +x /usr/local/bin/tini

RUN pip install requests
RUN pip install geojson

RUN mkdir /workspace
#RUN mkdir /workspace/volume
#RUN mkdir /workspace/MODELS


COPY run.sh /build/
RUN chmod +x /build/run.sh

RUN git clone https://github.com/GPlates/pygplates-tutorials.git /pygplates-tutorials

#RUN git clone https://github.com/siwill22/gwstools.git /gwstools

#RUN git clone https://github.com/siwill22/gws-examples.git /gws-examples


RUN mkdir /workspace/tutorials/
RUN cp -av /pygplates-tutorials/* /workspace/tutorials/

#RUN mkdir /workspace/web-service-examples
#RUN cp -av /gws-examples/python/* /workspace/web-service-examples

#ENV PYTHONPATH ${PYTHONPATH}:/gwstools

# setup space for working in
VOLUME /workspace/volume
#VOLUME /workspace/MODELS

# launch notebook
WORKDIR /workspace
EXPOSE 8888
ENTRYPOINT ["/usr/local/bin/tini", "--"]

CMD /build/run.sh
