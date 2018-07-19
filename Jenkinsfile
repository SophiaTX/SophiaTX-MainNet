#!groovy

////////////////////////////////////////
properties([parameters([booleanParam(defaultValue: false, description: '', name: 'build_as_testnet')])])

pipeline {
  options {
    buildDiscarder(logRotator(artifactNumToKeepStr: '20'))
  }
  environment {
    ARCHIVE_NAME = "sophiatx_" + "#" + "${env.BUILD_NUMBER}" + ".tar.gz"
    GENESIS_FILE = "genesis.json"
  }
  agent {
    label get_label_name()
  }
  stages {
    stage('Build') {
      steps {
        get_genesis_file_name()
        sh 'cmake -DUSE_PCH=ON -DBOOST_ROOT=${BOOST_160} -DOPENSSL_ROOT_DIR=${OPENSSL_102} -DFULL_STATIC_BUILD=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=install -DSOPHIATX_EGENESIS_JSON=${GENESIS_FILE}'
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
            dir('lib') {
                sh 'strip -s libalexandria.so libalexandriaJNI.so' //strip symbols
                sh 'tar -czf libalexandria.tar.gz libalexandria.so libalexandriaJNI.so alexandria.hpp AlexandriaJNI.java' //create tar file
                archiveArtifacts '*.gz'
            }
          dir('bin') {
              sh 'strip -s *' //strip symbols
              sh 'rm -f test*' //remove test binaries
              sh 'tar -czf ${ARCHIVE_NAME} *' //create tar file
              archiveArtifacts '*.gz'
          }
        }
      }
    }
    stage('Create RPM') {
      when {
          branch 'develop'
      }
     steps {
        sh 'rm -rf /home/$USER/RPMBUILD/RPMS/*.rpm'
        dir('install') {
          dir('bin') {
              sh 'mv *.gz sophiatx.tar.gz' //rename tar file
              sh 'cp sophiatx.tar.gz /home/$USER/RPMBUILD/SOURCES' //copy file for rpm creation
          }
        }
        sh 'cp ciscripts/sophiatx.spec /home/$USER/RPMBUILD/SPECS'
        sh 'rpmbuild -ba /home/$USER/RPMBUILD/SPECS/sophiatx.spec'
        sh 'cp /home/$USER/RPMBUILD/RPMS/x86_64/*.rpm ${WORKSPACE}'
        archiveArtifacts '*.rpm'
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
  if( "${env.BRANCH_NAME}" == 'develop' ) {
   slackSend (color: 'good', message: "SUCCESS: Job '${env.JOB_NAME} [${env.BUILD_NUMBER}]' (${env.BUILD_URL})")
  }
}

def get_label_name() {
  if( "${env.BRANCH_NAME}" == 'develop' ) {
    return 'suse'
  } else {
    return 'linux'
  }
}

def get_genesis_file_name() {
    script {
      if( ${params.build_as_testnet} ) {
        GENESIS_FILE = 'genesis_testnet.json'
      }
    }
}
