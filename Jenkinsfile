pipeline {
  // Build within a clean CentOS 6 container
  agent label:'docker', docker:'centos:6'
  stages {
    stage('prep') {
      steps {
        // Install the necessary RPM tools
        sh 'sudo yum install -y rpmdevtools yum-utils'
        // Install build dependencies for the target package
        sh 'sudo yum-builddep bbcollab-ptlib.spec'
        // Create the rpmbuild directory tree
        sh 'rpmdev-setuptree'
      }
    }
    stage('package') {
      steps {
        // Create our source tarball
        sh 'tar -czf ~/rpmbuild/SOURCES/zsdk-ptlib.src.tgz --exclude-vcs .'
        // Then let rpmbuild and the spec file handle the rest
        sh "rpmbuild -ta --define='jenkins_release .${env.BUILD_NUMBER}' ~/rpmbuild/SOURCES/zsdk-ptlib.src.tgz"
      }
      post {
        success {
          archive "~/rpmbuild/RPMS/**/*"
        }
      }
    }
    stage('publish-staging') {
      when {
        env.BRANCH == 'staging'
      }
      steps {
        // TODO
      }
    }
  }
  post {
    success {
      hipchatSend color: "GREEN", message: "Build Successful: ${env.JOB_NAME} ${env.BUILD_NUMBER}"
    }
    failure {
      hipchatSend color: "RED", message: "Build Failed: ${env.JOB_NAME} ${env.BUILD_NUMBER}"
    }
    unstable {
      hipchatSend color: "YELLOW", message: "Build Unstable: ${env.JOB_NAME} ${env.BUILD_NUMBER}"
    }
  }
}