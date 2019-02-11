#!groovy

////////////////////////////////////////


properties([parameters([booleanParam(defaultValue: false, description: '', name: 'build_as_debug'),
  booleanParam(defaultValue: false, description: '', name: 'build_as_testnet')])])


pipeline {
  options {
    buildDiscarder(logRotator(artifactNumToKeepStr: '5'))
    skipDefaultCheckout()
    parallelsAlwaysFailFast() 
  }
  environment {
    GENESIS_FILE = "genesis.json"
    BUILD_TYPE = "Release"
  }
  agent {
    label "linux"
  }
  stages {
    stage('Creating parallel jobs ...') {  
      parallel {
        stage('Linux') {
          agent {
            label "linux"
          }
          stages {
            stage('Git Checkout') {
              steps {
                checkout scm
              }
            }
            stage('Build') {
              steps {
                start_build()
              }
            }
            stage('Tests') {
              steps {
                tests()
              }
            }
            stage('Archive') {
              environment { 
                LIB_ARCHIVE_NAME = "libalexandria_" + "${env.NODE_NAME}" + ".tar.gz"
                ARCHIVE_NAME = "sophiatx_" + "${env.NODE_NAME}" +"_#" + "${env.BUILD_NUMBER}" + ".tar.gz"
              }
              steps {
                run_archive()
              }
            }
            stage('Create RPM') {
              when {
                branch 'develop'
              }
              steps {
                create_rpm()
              }
            }
            stage('Clean WS') {
              steps {
                cleanWs()
              }
            }
          }
        }
        stage('macOS') {    
          agent {
            label "mac"
          }
          when {
            anyOf {
              branch 'develop'
              branch "*/develop"
              branch "PR-*"
            }
          }
          stages {
            stage('Git Checkout') {
              steps {
                checkout scm
              }
            }
            stage('Build') {
              steps {
                start_build()
              }
            }
            stage('Tests') {
              steps {
                tests()
              }
            }
            stage('Archive') {
              environment { 
                LIB_ARCHIVE_NAME = "libalexandria_" + "${env.NODE_NAME}" + ".tar.gz"
                ARCHIVE_NAME = "sophiatx_" + "${env.NODE_NAME}" +"_#" + "${env.BUILD_NUMBER}" + ".tar.gz"
              }
              steps {
                run_archive()
              }
            }
            stage('Clean WS') {
              steps {
                cleanWs()
              }
            }
          }
        }
      }
    }
  }
  post {
    success {
      send_positive_slack_notification()
    }
    failure {
      slackSend (color: '#ff0000', message: "FAILED: Job '${env.JOB_NAME} [${env.BUILD_NUMBER}]' (${env.NODE_NAME}) (${env.BUILD_URL})")
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

def start_build() {
  script {
    if( params.build_as_testnet ) {
      GENESIS_FILE = "genesis_testnet.json"
    }
    if( params.build_as_debug ) {
      BUILD_TYPE = "Debug"
    }
  }
  sh "cmake . -DUSE_PCH=OFF -DBOOST_ROOT=${BOOST_167} -DOPENSSL_ROOT_DIR=${OPENSSL_111} -DSQLITE3_ROOT_DIR=${SQLITE_3253} -DSOPHIATX_STATIC_BUILD=ON -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DCMAKE_INSTALL_PREFIX=install -DSOPHIATX_EGENESIS_JSON=${GENESIS_FILE} -DBUILD_SOPHIATX_TESTNET=${params.build_as_testnet}"
  sh 'make -j4'
}

def tests() {
  script {
    if( !params.build_as_testnet ) {
      sh './tests/chain_test'
      sh './tests/plugin_test'
      sh './tests/smart_contracts/smart_contracts_tests'
      sh './tests/utilities/utilities_tests'
      sh './libraries/fc/vendor/secp256k1-zkp/src/project_secp256k1-build/tests'
      sh './libraries/fc/tests/all_tests'
      sh './libraries/SQLiteCpp/SQLiteCpp_tests'
    }
  }
}

def run_archive() {
  sh 'make install'
  dir('install') {
    dir('lib') {
      script {
        echo "${LIB_ARCHIVE_NAME}"
        if( !params.build_as_debug ) {
          try {
              sh 'strip -s libalexandria.so libalexandriaJNI.so' //strip symbols
              } catch(Exception e) {
                echo "Skipping strip on macOS"
              }
            }
          }
      sh "tar -czf ${LIB_ARCHIVE_NAME} libalexandria.so libalexandriaJNI.so alexandria.hpp AlexandriaJNI.java" //create tar file
      archiveArtifacts '*.gz'
    }
  dir('bin') {
    sh 'rm -f test*' //remove test binaries
    script {
      echo "${ARCHIVE_NAME}"
      if( !params.build_as_debug ) {
        try {
            sh 'strip -s *' //strip symbols
            } catch(Exception e) {
              echo "Skipping strip on macOS"
            }
          }

          if( params.build_as_testnet ) {
           sh "cp ${WORKSPACE}/contrib/testnet_config.ini ."//copy config
           sh "tar -czf ${ARCHIVE_NAME} cli_wallet sophiatxd sophiatxd_light testnet_config.ini" //create tar file
           } else {
           sh "cp ${WORKSPACE}/contrib/fullnode_config.ini ."//copy configs
           sh "cp ${WORKSPACE}/contrib/witness_config.ini ."//copy configs
           sh "tar -czf ${ARCHIVE_NAME} cli_wallet sophiatxd sophiatxd_light fullnode_config.ini witness_config.ini" //create tar file
         }
       }
       archiveArtifacts '*.gz'
     }
   }
 }

def create_rpm() {
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
