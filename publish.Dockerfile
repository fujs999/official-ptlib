#!docker

FROM amazon/aws-cli
RUN yum update --assumeyes && \
    yum install --assumeyes yum-utils createrepo && \
    yum clean all
ENTRYPOINT [""]
