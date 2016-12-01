FROM centos:6
RUN yum groupinstall -y "Development tools" && yum clean all
RUN yum install -y \
    rpmdevtools \
    yum-utils \
    && yum clean all
RUN yum install -y epel-release && yum clean all
RUN groupadd -g 1000 jenkins && useradd -M -u 1000 -g 1000 jenkins
COPY mcu.repo /etc/yum.repos.d/
COPY bbcollab-ptlib.spec .
# Use two-step build dependency installation, so we can cache the RPMs for fingerprinting
RUN yum-builddep -y --downloadonly --downloaddir=/var/cache/jenkins/build-deps bbcollab-ptlib.spec && yum clean all
RUN yum install -y /var/cache/jenkins/build-deps/*.rpm && yum clean all