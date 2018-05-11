#!groovy
pipeline {
  agent any
  stages {
    stage('Build') {
      steps {
        sh 'cmake -DBOOST_ROOT=$BOOST_160 -DOPENSSL_ROOT_DIR=$OPENSSL_102'
        sh 'make -j4'
      }
    }
    stage('Tests') {
      steps {
        sh './tests/chain_test'
      }
    stage('Clean WS') {
      steps {
        cleanWs()
      }
    }
  }
  post {
    success {
      echo 'TODO'
      //sh 'ciscripts/buildsuccess.sh'
    }
    failure {
      echo 'TODO'
      //sh 'ciscripts/buildfailure.sh'
      slackSend (color: '#ff0000', message: "FAILED: Job '${env.JOB_NAME} [${env.BUILD_NUMBER}]' (${env.BUILD_URL})")
    }
  }
}
