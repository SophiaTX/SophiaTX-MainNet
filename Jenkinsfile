#!groovy
archive_name = "sophiatx_" + "${env.BUILD_NUMBER}" + "tar.gz"

////////////////////////////////////////
pipeline {
  options {
    buildDiscarder(logRotator(artifactNumToKeepStr: '20'))
  }
  agent { label 'suse' }
  stages {
    stage('Build') {
      steps {
        sh 'cmake -DBOOST_ROOT=${BOOST_160} -DOPENSSL_ROOT_DIR=${OPENSSL_102} -DFULL_STATIC_BUILD=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=install'
        sh 'make -j4'
      }
    }
    stage('Tests') {
      steps {
        sh './tests/chain_test'
      }
    }
    stage('Archive') {
     steps {
        sh 'make install'
        dir('install') {
          dir('bin') {
              sh 'strip -s *' //strip symbols
              sh 'rm -f test*' //remove test binaries
              sh 'tar -czf ${archive_name} *' //create tar file
              archiveArtifacts '*.gz'
          }
        }
      }
    }
    stage('Clean WS') {
      steps {
        cleanWs()
      }
    }
  }
  post {
    success {
      send_positive_slack_notification()
    }
    failure {
      slackSend (color: '#ff0000', message: "FAILED: Job '${env.JOB_NAME} [${env.BUILD_NUMBER}]' (${env.BUILD_URL})")
    }
  }
}
////////////////////////////////////////

def send_positive_slack_notification() {
  if( "${env.BRANCH_NAME}" == 'feature/build-w-artifacts' ) {
   slackSend (color: 'good', message: "SUCCESS: Job '${env.JOB_NAME} [${env.BUILD_NUMBER}]' (${env.BUILD_URL})")
  }
}