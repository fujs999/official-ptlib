FROM centos:7 AS base
# Add standard tools expected on any development machine
RUN yum groupinstall -y "Development tools" && yum clean all
# Add tools needed for RPM building and dealing with yum repositories
RUN yum install -y rpmdevtools yum-utils && yum clean all
# Set up a non-root user for RPM builds
ARG USERID=1000
ARG GROUPID=1000
RUN groupadd -g ${USERID} rpmbuild && useradd -u ${USERID} -g ${GROUPID} rpmbuild
WORKDIR /home/rpmbuild
# Add yum repositories needed for our builds
RUN yum install -y epel-release centos-release-scl-rh && yum clean all
# Install large/common dependencies (to avoid delays upon later cache invalidation)
RUN yum install -y --setopt=tsflags=nodocs devtoolset-9-gcc devtoolset-9-gcc-c++ && yum clean all
RUN yum install -y ImageMagick-devel openssl-devel && yum clean all
# Configure our own yum repos
COPY mcu-el7.repo /etc/yum.repos.d/
# Uncomment if you want to install some local dependencies instead of using Nexus yum repos
#COPY build-deps /tmp/build-deps/
#RUN yum install -y /tmp/build-deps/*.rpm && yum clean all


FROM base AS depsolver
ARG SPECFILE
COPY ${SPECFILE} .
# Install standard dependencies referenced by the spec file
RUN mkdir /tmp/std-deps \
    && touch /tmp/std-deps/placeholder \
    && yum-builddep --exclude='bbcollab-*' --exclude='bbrtc-*' --exclude='boost164-devel' \
                    --exclude='libsrtp2-devel' --exclude='opus-devel' --tolerant \
                    -y --downloadonly --downloaddir=/tmp/std-deps ${SPECFILE} \
    && ([ -z "$(ls -A /tmp/std-deps/*.rpm)" ] || yum install -y /tmp/std-deps/*.rpm) \
    && yum clean all
# Invalidate Docker cache if our yum repo metadata has changed (don't care about standard repos)
ADD http://nexus.bbcollab.net/yum/el7/mcu-release/repodata/repomd.xml /tmp/mcu-release.xml
ADD http://nexus.bbcollab.net/yum/el7/mcu-develop/repodata/repomd.xml /tmp/mcu-develop.xml
# Download internal dependencies referenced by the spec file
RUN mkdir /tmp/build-deps \
    && touch /tmp/build-deps/placeholder \
    && yum-builddep -y --downloadonly --downloaddir=/tmp/build-deps ${SPECFILE} \
    && yum clean all


FROM base
# Install standard dependencies
COPY --from=depsolver /tmp/std-deps/* /tmp/std-deps/
RUN [ -z "$(ls -A /tmp/std-deps/*.rpm)" ] || (yum install -y /tmp/std-deps/*.rpm && yum clean all)
# Install internal dependencies
COPY --from=depsolver /tmp/build-deps/* /tmp/build-deps/
RUN [ -z "$(ls -A /tmp/build-deps/*.rpm)" ] || (yum install -y /tmp/build-deps/*.rpm && yum clean all)
USER rpmbuild
