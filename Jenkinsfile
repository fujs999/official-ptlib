#!groovy

@Library('collab-jenkins-library') _

def spec_file = 'ptlib.spec'
def job_name = JOB_NAME.replaceAll('%2F', '_')
def build_tag = BUILD_TAG.replaceAll('%2F', '-')
def el7_builder = null

pipeline {
  agent any
  stages {
    stage('el7-builder') {
      // This avoids the matrix trying to build the docker image in parallel
      steps {
        script {
          el7_builder = docker.build("el7_builder:${build_tag}", "--build-arg SPECFILE=${spec_file} --file el7.Dockerfile .")
        }
      }
    }
    stage('matrix') {
      failFast true
      matrix {
        axes {
          axis {
            name 'DIST'
            values 'el7', 'amzn2'
          }
          axis {
            name 'REPO'
            values 'mcu-develop', 'mcu-release', 'mcu-release-tsan'
          }
          axis {
            name 'ARCH'
            values 'x86_64', 'aarch64'
          }
        }
        when {
          anyOf {
            allOf {
              branch 'develop'
              expression { return REPO == 'mcu-develop' }
            }
            allOf {
              branch 'release/*'
              expression { return REPO != 'mcu-develop' }
            }
          }
        }
        excludes {
          exclude {
            axis {
              name 'DIST'
              values 'el7'
            }
            axis {
              name 'ARCH'
              values 'aarch64'
            }
          }
          exclude {
            axis {
              name 'DIST'
              values 'amzn2'
            }
            axis {
              name 'REPO'
              values 'mcu-release-tsan'
            }
          }
        }
        environment {
          HOME = "${WORKSPACE}/${DIST}-${REPO}"
        }
        stages {
          stage('package') {
            steps {
              script {
                sh 'mkdir -p $HOME'
                if (DIST == 'el7') {
                  el7_builder.inside() {
                    sh "SPECFILE=${spec_file} ./rpmbuild.sh --with=${REPO.replace('mcu-release-','')}"
                  }
                }
                else {
                  awsCodeBuild \
                      region: env.AWS_REGION, sourceControlType: 'jenkins', \
                      credentialsId: 'aws-codebuild', credentialsType: 'jenkins', sseAlgorithm: 'AES256', \
                      cloudWatchLogsStatusOverride: 'ENABLED', cloudWatchLogsGroupNameOverride: 'bbrtc-codebuild', \
                      cloudWatchLogsStreamNameOverride: "${job_name}/${ARCH}", \
                      artifactTypeOverride: 'S3', artifactLocationOverride: 'bbrtc-codebuild', \
                      artifactNameOverride: 'rpmbuild', artifactPathOverride: build_tag, \
                      downloadArtifacts: 'true', downloadArtifactsRelativePath: '.', \
                      envVariables: "[ { BUILD_NUMBER, ${BUILD_NUMBER} }, { BRANCH_NAME, ${BRANCH_NAME} }, { SPECFILE, ${spec_file} } ]", \
                      projectName: "BbRTC-${ARCH}"
                  sh "mkdir -p ${HOME}/rpmbuild/RPMS/${ARCH}"
                  sh "mv ${build_tag}/rpmbuild/RPMS/${ARCH}/* ${HOME}/rpmbuild/RPMS/${ARCH}"
                }
              }
            }
            post {
              success {
                archiveArtifacts artifacts: "${DIST}-${REPO}/rpmbuild/RPMS/**/*", fingerprint: true
              }
            }
          }
          stage('publish') {
            steps {
              unarchive mapping:["${DIST}-${REPO}/rpmbuild/RPMS/" : '.']
              script {
                stageYumPublish rpms: "${HOME}/rpmbuild/RPMS", dist: DIST, repo: REPO, arch: ARCH, local_path: "${HOME}/yum"
              }
            }
          }
        }
      }
    }
  }

  post {
    success {
      slackSend color: "good", message: "SUCCESS: <${currentBuild.absoluteUrl}|${currentBuild.fullDisplayName}>"
    }
    failure {
      slackSend color: "danger", message: "@channel FAILURE: <${currentBuild.absoluteUrl}|${currentBuild.fullDisplayName}>"
    }
    unstable {
      slackSend color: "warning", message: "@channel UNSTABLE: <${currentBuild.absoluteUrl}|${currentBuild.fullDisplayName}>"
    }
  }
}
