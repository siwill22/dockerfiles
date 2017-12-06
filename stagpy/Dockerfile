##
# Image for pygplates Jupyter server
#

FROM ubuntu:16.10

RUN apt-get update

RUN apt-get install -y python3.6

# install wget to enable the pygplates source code to be downloaded from sourceforge
RUN apt-get update
RUN apt-get install -y git


#### Python dependencies
# install all the python and ipython notebook requirements
RUN apt-get install -y gcc python3-pip
RUN apt-get install -y python3-numpy python3-scipy python3-matplotlib python3-pandas
#RUN pip install --upgrade --no-use-wheel pip
RUN pip3 install --upgrade setuptools
RUN pip3 install jupyter
RUN apt-get install -y python3-venv
RUN pip3 install setuptools_scm argcomplete
RUN pip3 install f90nml

RUN apt-get install -y cmake
RUN git clone https://github.com/StagPython/StagPy.git /StagPy
RUN cd /StagPy && make

ENV PYTHONPATH ${PYTHONPATH}:/StagPy/

ENV TINI_VERSION v0.8.4
ADD https://github.com/krallin/tini/releases/download/${TINI_VERSION}/tini /usr/local/bin/tini
RUN chmod +x /usr/local/bin/tini


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
