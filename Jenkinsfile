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
    stage('Clean WS') {
     steps {
        cleanWs()
      }
    }
  }
  post {
    success {
      setBuildStatus("Build complete", "SUCCESS");
    }
    failure {
      setBuildStatus("Build Failed", "FAILURE");
      slackSend (color: '#ff0000', message: "FAILED: Job '${env.JOB_NAME} [${env.BUILD_NUMBER}]' (${env.BUILD_URL})")
    }
  }
}

void setBuildStatus(String message, String state) {
  step([
      $class: "GitHubCommitStatusSetter",
      reposSource: [$class: "ManuallyEnteredRepositorySource", url: "https://github.com/SophiaTX/SophiaTX.git"],
      contextSource: [$class: "ManuallyEnteredCommitContextSource", context: "ci/jenkins/build-status"],
      errorHandlers: [[$class: "ChangingBuildStatusErrorHandler", result: "UNSTABLE"]],
      statusResultSource: [ $class: "ConditionalStatusResultSource", results: [[$class: "AnyBuildResult", message: message, state: state]] ]
  ]);
}