# zsdk-ptlib: Portable Tools Library

## Building

### Local build

The local build process has not changed, see [ReadMe.txt](ReadMe.txt) for details.

### Local RPM build

The local build is performed using `mock`, which ensures a consistent build
environment by setting up a chroot. It uses `yum` to install essential packages
within the chroot, plus any packages listed as build dependenciesin the spec
file.

    ./mockbuild.sh

The provided mock config file, `mcu-epel-6-x86_64.cfg`, adds MCU yum
repository definitions, though only the release repository is enabled by
default. If you need a newer, unreleased version of an MCU dependency,
additional repositories can be enabled by adding an `--enablerepo` flag to the
`mock` command, e.g. `--enablerepo=mcu-develop`.

### Jenkins build

The [Jenkins build][1] uses Docker to ensure a consistent build environment. The
build process is configured by the `Jenkinsfile`, which delegates Docker
container setup to the `Dockerfile`. The `Jenkinsfile` also handles publishing
certain branches to yum repostitories on Nexus, and notifying HipChat of build
results.

## Versioning

The PTLib version number should follow the rules of [Semantic Versioning][2].
Every merge (or direct commit) to the integration branch should include a
version number update to the spec file.

Note that the spec file is now considered the master copy of the version number,
so the RPM build process should use this in favour of any version numbers
elsewhere in the code. If you perform a local build, you may need to adjust any
built-in version manually.

The RPM release number should always remain at 1 on the integration branch. The
number should be set to 2 in the first commit on a release branch, providing
an easy way to distinguish a develop RPM from a release RPM of the same verison.

Note that Jenkins appends its build number to the release number, so it should
be easy to distinguish RPMs that contain different code vs RPMs that are
different builds of the same code. If an RPM has been built outside of Jenkins
the build number component should be missing.

For example:

    bbcollab-ptlib-2.17.1-1.2.el6.x86_64.rpm
    package        ver    rel     arch

* Package: bbcollab-ptlib
* Version: 2.17.1
* Release: 1.2.el6 (Release increment: 1, Jenkins build number: 2, Dist tag: el6)
* Arch: x86_64

## Branching

Loosely follow the [GitFlow][3] model, but with the following modifications:
* There is no "production" branch (normally `master` in Git Flow)
* The integration branch is named `staging` instead of `develop`
* `master` and `develop` branches do exist, but are lost in history somewhere
  and should be ignored

Once we have fully switched to the new Jenkins we should look at cleaning up our
branches and agree the model/naming to use. For now, we don't want to break the
builds on the existing Jenkins by changing everything.

## Releasing

1. Create a new release branch named `release/<YYYY-MM>` from an appropriate
   position on the integration branch.
1. Update the Release number in the RPM spec file to 2 (should always be 1 on
   the integration branch).
1. Update the build dependencies in the RPM spec file to require exact versions.
   This makes it easier to go back and recreate a specific release build at a
   later date, even if newer versions of the build dependencies have been
   released.
1. Commit and push the spec file changes. Jenkins will be notified of the
   change, build the new release branch, and publish the output RPM to the
   `mcu-release` yum repository on Nexus.
1. Perform appropriate testing of the release candidate, and apply additional
   commits to the branch as needed. Non-trivial fixes should be performed on an
   additional `bugfix/` branch (with an increment to the Release number in the
   spec file) to allow for code review.
1. When testing has completed, tag the final release commit.
1. Once the release is tagged, merge it back into the integration branch as
   follows:

       git checkout staging
       git pull
       git merge --no-ff --no-commit release/2016-12

   This allows you to make some changes before committing and pushingthe merge,
   effectively reverting the first commit on the release branch:
   * Reset the Release number to 1
   * Restore the normal build dependency version requirements (not exact)

[1]: http://collab-jenkins.bbpd.io/job/zsdk-ptlib/
[2]: http://semver.org/
[3]: http://nvie.com/posts/a-successful-git-branching-model/
