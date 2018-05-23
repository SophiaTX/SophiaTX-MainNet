#!groovy
pipeline {
  agent { label 'suse' }
  stages {
    stage('Build') {
      steps {
        sh 'cmake -DBOOST_ROOT=$BOOST_160 -DOPENSSL_ROOT_DIR=$OPENSSL_102 -DFULL_STATIC_BUILD=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=install'
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
                sh 'tar -cf sophiatx.tar *' //create tar file
                archive '*.tar'
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
      echo env.BRANCH_NAME
    }
    failure {
      slackSend (color: '#ff0000', message: "FAILED: Job '${env.JOB_NAME} [${env.BUILD_NUMBER}]' (${env.BUILD_URL})")
    }
  }
}
