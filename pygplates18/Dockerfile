##
# Image for pygplates basic dependencies, ubuntu 18
#

FROM ubuntu:18.04

RUN apt-get update

# install dependencies for pygplates
RUN apt-get install -y libglew-dev
RUN apt-get install -y python2.7-dev
RUN apt-get install -y libboost-dev libboost-python-dev libboost-thread-dev libboost-program-options-dev libboost-test-dev libboost-system-dev
RUN apt-get install -y libqt4-dev
RUN apt-get install -y libgdal-dev
RUN apt-get install -y libcgal-dev
RUN apt-get install -y libproj-dev
RUN apt-get install -y libqwt-dev
RUN apt-get install -y libxrender-dev libice-dev libsm-dev libfreetype6-dev libfontconfig1-dev

# install wget to enable the pygplates source code to be downloaded from sourceforge
RUN apt-get update
RUN apt-get install -y wget git

# use wget to get the correct pygplates package from sourceforge
COPY pygplates_2.0_amd64_bionic.deb pygplates_2.0_amd64_bionic.deb

# use dpkg to install 
RUN dpkg -i pygplates_2.0_amd64_bionic.deb 

# note the revision number here may need changing if the version of pygplates is incremented
ENV PYTHONPATH ${PYTHONPATH}:/usr/lib:/usr/lib/pygplates/revision17/



