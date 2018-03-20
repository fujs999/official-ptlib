#!/bin/sh
# Build script for local testing of RPM build process; not used by Jenkins

set -e

if ! which mock > /dev/null; then
    echo You must install mock first: sudo yum install -y mock
    exit 1
fi

if ! which spectool > /dev/null; then
    echo You must install spectool first: sudo yum install -y rpmdevtools
    exit 1
fi

# Build the source RPM
mkdir -p rpmbuild/SOURCES
tar -czf rpmbuild/SOURCES/zsdk-ptlib.src.tgz --exclude-vcs --exclude=rpmbuild .
mock --root=mcu-epel-6-x86_64.cfg $@ --buildsrpm --spec bbcollab-ptlib.spec --sources rpmbuild/SOURCES --resultdir rpmbuild/SRPMS

# Build the final RPMs
mock --root=mcu-epel-6-x86_64.cfg $@ --resultdir rpmbuild/RPMS rpmbuild/SRPMS/bbcollab-ptlib-*.el6.src.rpm

