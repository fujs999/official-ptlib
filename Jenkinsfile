pipeline {
  agent none
  stages {
    stage('package') {
      // Build environment is defined by the Dockerfile
      agent {
        dockerfile {
          additionalBuildArgs  '--build-arg SPECFILE=bbcollab-ptlib.spec'
          customWorkspace "${JOB_NAME.replaceAll('%2F', '_')}"
        }
      }
      steps {
        // Copy RPM dependencies to the workspace for fingerprinting (see Dockerfile)
        sh 'cp -r /tmp/build-deps .'
        sh './rpmbuild.sh'
      }
      post {
        success {
          fingerprint 'build-deps/*.rpm'
          archiveArtifacts artifacts: 'rpmbuild/**/*', fingerprint: true
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
