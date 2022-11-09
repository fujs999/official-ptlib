#!docker

# See https://docs.aws.amazon.com/linux/al2022/ug/install-docker.html
FROM amazonlinux:2022

# Add standard tools expected on any development machine
RUN yum install --assumeyes shadow-utils sudo gcc-c++ git libasan-static libtsan-static && yum clean all

# Add tools needed for RPM building and dealing with yum repositories
RUN yum install --assumeyes rpmdevtools yum-utils && yum clean all

# Install dependencies referenced by the spec file
ARG SPECFILE
COPY ${SPECFILE} .
RUN yum-builddep --assumeyes ${SPECFILE} && yum clean all

# Set up a non-root user for RPM builds
ARG USERID=1000
ARG GROUPID=1000
ARG USERNAME=rpmbuild
RUN groupadd -g ${USERID} ${USERNAME} && useradd -u ${USERID} -g ${GROUPID} ${USERNAME}
WORKDIR /home/${USERNAME}
USER ${USERNAME}
RUN rpmdev-setuptree
