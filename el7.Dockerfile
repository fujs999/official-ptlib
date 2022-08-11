FROM centos:7

# Add standard tools expected on any development machine
RUN yum groupinstall -y "Development tools" && yum clean all

# Add tools needed for RPM building and dealing with yum repositories
RUN yum install -y rpmdevtools yum-utils && yum clean all

# Add yum repositories needed for our builds
RUN yum install -y epel-release centos-release-scl-rh && yum clean all

# Install large/common dependencies (to avoid delays upon later cache invalidation)
RUN yum install -y --setopt=tsflags=nodocs \
        devtoolset-9-gcc \
        devtoolset-9-gcc-c++ \
        devtoolset-9-libtsan-devel \
        devtoolset-9-libasan-devel && \
    yum clean all

# Install dependencies referenced by the spec file
ARG SPECFILE
COPY ${SPECFILE} .
RUN yum-builddep -y ${SPECFILE} && yum clean all

# Set up a non-root user for RPM builds
ARG USERID=1000
ARG GROUPID=1000
RUN groupadd -g ${USERID} rpmbuild && useradd -u ${USERID} -g ${GROUPID} rpmbuild
WORKDIR /home/rpmbuild
USER rpmbuild
