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
  }
  post {
    success {
      //sh 'ciscripts/buildsuccess.sh'
    }
    failure {
      //sh 'ciscripts/buildfailure.sh'
      //slackSend (color: '#ff0000', message: "FAILED: Job '${env.JOB_NAME} [${env.BUILD_NUMBER}]' (${env.BUILD_URL})")
    }
  }
}
