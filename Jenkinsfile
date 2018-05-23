#!groovy
pipeline {
  agent { label 'suse' }
  stages {
    stage('Build') {
      steps {
        sh 'cmake -DBOOST_ROOT=$BOOST_160 -DOPENSSL_ROOT_DIR=$OPENSSL_102 -DFULL_STATIC_BUILD=ON'
        sh 'make -j4'
      }
    }
    stage('Tests') {
      steps {
        sh './tests/chain_test'
      }
    }
    stage('Archive')
     steps {
        echo 'TODO'
        //sh 'make install'
     }
    stage('Clean WS') {
      steps {
        echo 'TODO'
        //cleanWs()
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
