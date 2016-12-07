pipeline {
  // Build environment is defined by the Dockerfile
  agent dockerfile:true
  stages {
    stage('package') {
      steps {
        // Create the rpmbuild directory tree
        sh "HOME=$WORKSPACE rpmdev-setuptree"
        sh "HOME=$WORKSPACE rpmdev-wipetree"
        // Create our source tarball
        sh 'tar -czf rpmbuild/SOURCES/zsdk-ptlib.src.tgz --exclude-vcs --exclude=rpmbuild .'
        // Then let rpmbuild and the spec file handle the rest
        sh "HOME=$WORKSPACE rpmbuild -ta --define='jenkins_release .${env.BUILD_NUMBER}' rpmbuild/SOURCES/zsdk-ptlib.src.tgz"
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
      when {
        env.BRANCH_NAME == 'staging'
      }
      steps {
        build job: '/rpm-repo-deploy', parameters: [string(name: 'SOURCE_JOB', value: "$JOB_NAME"), string(name: 'SOURCE_BUILD', value: "$BUILD_NUMBER"), string(name: 'DEST_REPO', value: 'mcu-develop')], quietPeriod: 0
      }
    }
    stage('publish-release') {
      when {
        env.BRANCH_NAME ==~ /release\/.*/
      }
      steps {
        build job: '/rpm-repo-deploy', parameters: [string(name: 'SOURCE_JOB', value: "$JOB_NAME"), string(name: 'SOURCE_BUILD', value: "$BUILD_NUMBER"), string(name: 'DEST_REPO', value: 'mcu-release')], quietPeriod: 0
      }
    }
  }
  post {
    success {
      hipchatSend color: "GREEN", message: "SUCCESS: <a href=\"${currentBuild.absoluteUrl}\">${currentBuild.fullDisplayName}</a>"
    }
    failure {
      hipchatSend color: "RED", message: "FAILURE: <a href=\"${currentBuild.absoluteUrl}\">${currentBuild.fullDisplayName}</a>", notify: true
    }
    unstable {
      hipchatSend color: "YELLOW", message: "UNSTABLE: <a href=\"${currentBuild.absoluteUrl}\">${currentBuild.fullDisplayName}</a>", notify: true
    }
  }
}