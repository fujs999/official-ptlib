pipeline {
  agent none
  stages {
    stage('package') {
      // Build environment is defined by the Dockerfile
      agent { dockerfile true }
      steps {
        // Create the rpmbuild directory tree
        sh "HOME=$WORKSPACE rpmdev-setuptree"
        sh "HOME=$WORKSPACE rpmdev-wipetree"
        // Create our source tarball
        sh 'tar -czf rpmbuild/SOURCES/zsdk-ptlib.src.tgz --exclude-vcs --exclude=rpmbuild .'
        // Then let rpmbuild and the spec file handle the rest
        sh "HOME=$WORKSPACE rpmbuild -ta --define='jenkins_release .${env.BUILD_NUMBER}' rpmbuild/SOURCES/zsdk-ptlib.src.tgz"
        // Copy build dependencies to the workspace for fingerprinting
        sh 'rm -rf build-deps'
        sh 'cp -r /var/cache/jenkins/build-deps .'
      }
      post {
        success {
          fingerprint 'build-deps/*.rpm'
          archive 'rpmbuild/RPMS/**/*'
        }
      }
    }
    stage('publish-develop') {
      when { branch 'develop' }
      steps {
        build job: '/rpm-repo-deploy', parameters: [
          string(name: 'SOURCE_JOB', value: "$JOB_NAME"),
          string(name: 'SOURCE_BUILD', value: "$BUILD_NUMBER"),
          string(name: 'DEST_REPO', value: 'mcu-develop')
        ], quietPeriod: 0
      }
    }
    stage('publish-release') {
      when { branch 'release/*' }
      steps {
        build job: '/rpm-repo-deploy', parameters: [
          string(name: 'SOURCE_JOB', value: "$JOB_NAME"),
          string(name: 'SOURCE_BUILD', value: "$BUILD_NUMBER"),
          string(name: 'DEST_REPO', value: 'mcu-release')
        ], quietPeriod: 0
      }
    }
  }
  post {
    success {
      slackSend color: "good", message: "SUCCESS: <${currentBuild.absoluteUrl}|${currentBuild.fullDisplayName}>"
    }
    failure {
      slackSend color: "danger", message: "FAILURE: <${currentBuild.absoluteUrl}|${currentBuild.fullDisplayName}>"
    }
    unstable {
      slackSend color: "warning", message: "UNSTABLE: <${currentBuild.absoluteUrl}|${currentBuild.fullDisplayName}>"
    }
  }
}
