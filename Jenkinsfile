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
    always {
      updateGithubCommitStatus()
    }
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


def getRepoURL() {
  sh "git config --get remote.origin.url > .git/remote-url"
  return readFile(".git/remote-url").trim()
}

def getCommitSha() {
  sh "git rev-parse HEAD > .git/current-commit"
  return readFile(".git/current-commit").trim()
}

def updateGithubCommitStatus(build = currentBuild) {
  // workaround https://issues.jenkins-ci.org/browse/JENKINS-38674
  repoUrl = getRepoURL()
  commitSha = getCommitSha()

  step([
    $class: 'GitHubCommitStatusSetter',
    reposSource: [$class: "ManuallyEnteredRepositorySource", url: repoUrl],
    commitShaSource: [$class: "ManuallyEnteredShaSource", sha: commitSha],
    errorHandlers: [[$class: 'ShallowAnyErrorHandler']],
    statusResultSource: [
      $class: 'ConditionalStatusResultSource',
      results: [
        [$class: 'BetterThanOrEqualBuildResult', result: 'SUCCESS', state: 'SUCCESS', message: build.description],
        [$class: 'BetterThanOrEqualBuildResult', result: 'FAILURE', state: 'FAILURE', message: build.description],
        [$class: 'AnyBuildResult', state: 'FAILURE', message: 'Loophole']
      ]
    ]
  ])
}