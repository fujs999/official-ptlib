#!groovy

def s3_publish(String rpms, String repo, String dist = 'el7', String arch = 'x86_64', dockerfile = 'publish.Dockerfile') {
  def local_path = './local_repo'
  def s3_path = "s3://citc-artifacts/yum/${dist}/${repo}/"
  echo "Publishing ${rpms} (${arch}) to ${s3_path}"
  if (!fileExists(file: dockerfile)) {
    sh """
      echo 'FROM amazon/aws-cli' > ${dockerfile}
      echo 'RUN yum install --assumeyes yum-utils createrepo' >> ${dockerfile}
    """
  }
  lock('aws-s3-citc-artifacts') {
    withAWS(credentials: 'aws-dev', region: 'us-east-1') {
      docker.build("s3-publish:${BUILD_TAG.replaceAll('%2F', '-')}", "-f ${dockerfile} .").inside("--entrypoint=''") {
        sh """
          aws s3 sync --no-progress --acl public-read ${s3_path} ${local_path}
          repomanage --keep=5 --old ${local_path} | xargs rm -f no_such_file_as_this_to_prevent_error
          mv ${rpms}/${arch}/* ${local_path}/base/
          createrepo --update ${local_path}
          aws s3 sync --no-progress --acl public-read --delete ${local_path} ${s3_path}
        """
      }
    }
  }
}

def tag_release(String spec_file) {
  env.SPEC_FILE = spec_file
  env.GIT_PATH = GIT_URL.replace('https://', '')
  env.RELEASE_TAG = "${BRANCH_NAME.replaceAll('release/', '')}-2.${BUILD_NUMBER}"
  echo "Tagging ${env.RELEASE_TAG}"
  withCredentials([usernamePassword(credentialsId: 'github-app-class-collab', passwordVariable: 'GIT_TOKEN', usernameVariable: 'GIT_USER')]) {
    sh '''
      major=`sed -n 's/%global *version_major *//p' $SPEC_FILE`
      minor=`sed -n 's/%global *version_minor *//p' $SPEC_FILE`
      patch=`sed -n 's/%global *version_patch *//p' $SPEC_FILE`
      oem=`  sed -n 's/%global *version_oem *//p'   $SPEC_FILE`
      git tag \$major.\$minor.\$patch.\$oem-2.$BUILD_NUMBER
      git tag $RELEASE_TAG
      git push --tags "https://$GIT_USER:$GIT_TOKEN@$GIT_PATH"
    '''
  }
}

pipeline {
  agent any

  options {
    buildDiscarder logRotator(artifactDaysToKeepStr: '', artifactNumToKeepStr: '', daysToKeepStr: '200', numToKeepStr: '200')
  }

  stages {
    stage('package-el7') {
      // Build environment is defined by the Dockerfile
      agent {
        dockerfile {
          filename 'el7.Dockerfile'
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

    stage('publish') {
      when {
        beforeAgent true
        anyOf {
          branch 'develop'
          branch 'release/*'
        }
      }
      steps {
        unarchive mapping:['rpmbuild/' : '.']
        script {
          s3_publish 'rpmbuild/RPMS', BRANCH_NAME == 'develop' ? 'mcu-develop' : 'mcu-release'
        }
      }
      post {
        success {
          script {
            if (BRANCH_NAME == 'develop') {
              build job: "/rpm-opal/develop", quietPeriod: 60, wait: false
            }
          }
        }
      }
    }

    stage('tag-release') {
      when {
        branch 'release/*'
      }
      steps {
        script {
          tag_release 'bbcollab-ptlib.spec'
        }
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
