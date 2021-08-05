#!docker

FROM centos:7
RUN yum update --assumeyes && \
    yum install --assumeyes yum-utils createrepo awscli && \
    yum clean all
