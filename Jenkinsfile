#!groovy

import com.cwctravel.hudson.plugins.extended_choice_parameter.ExtendedChoiceParameterDefinition

////////////////////////////////////////

properties([parameters([booleanParam(defaultValue: false, description: 'Build in debug mode', name: 'Debug'),
                        checkBox("Network", "Mainnet,Testnet,Customnet", "Testnet" /*default*/, 0, "PT_SINGLE_SELECT", "Select network"),
                        string(defaultValue: "", description: 'Custom genesis URL(Valid only for \"Customnet\" Network)', name: 'GenesisURL'),
                        checkBox("Package", "sophiatx,light-client,cli-wallet", "" /*default*/, 0, "PT_CHECKBOX", "Select packages to be built")
                      ])
          ])


pipeline {
  options {
    buildDiscarder(logRotator(artifactNumToKeepStr: '5'))
    skipDefaultCheckout()
    parallelsAlwaysFailFast() 
  }
  environment {
    GENESIS_FILE = ""
    BUILD_TESTNET = ""
    BUILD_TYPE = ""
  }
  agent any
  stages {
    stage('Init build variables') {
      steps {
        init()
      }
    }
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
    stage('Package') {
      steps {
        create_packages()
      }
    }
    stage('Archive') {
      environment {
        LIB_ARCHIVE_NAME = "libalexandria_" + "${env.NODE_NAME}" + ".tar.gz"
        ARCHIVE_NAME = "sophiatx_" + "${env.NODE_NAME}" +"_#" + "${env.BUILD_NUMBER}" + ".tar.gz"
        PLUGIN_ARCHIVE_NAME = "plugins_" + "${env.NODE_NAME}" +"_#" + "${env.BUILD_NUMBER}" + ".tar.gz"
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
////////////////////////////////////////


def init() {
    if( params.Debug ) {
      BUILD_TYPE = "Debug"
    } else {
      BUILD_TYPE = "Release"
    }

    if( params.Network == "Mainnet" ) {
      BUILD_TESTNET = "false"
      GENESIS_FILE = "${WORKSPACE}/libraries/egenesis/genesis.json"
    } else if( params.Network == "Testnet" ) {
      BUILD_TESTNET = "true"
      GENESIS_FILE = "${WORKSPACE}/libraries/egenesis/genesis_testnet.json"
    } else if( params.Network == "Customnet" ) {
      BUILD_TESTNET = "false"
      if (params.GenesisURL == "") {
        error("Genesis URL must be provided to build Custom network...")
      }

      GENESIS_FILE = "${WORKSPACE}/libraries/egenesis/custom_genesis.json"

      try {
        sh "rm -f ${GENESIS_FILE}"
      } catch(Exception e) {
        echo "Skipping removing existing(previous) custom genesis file. It does not exist."
      }

      try {
        sh "wget --output-document=${GENESIS_FILE} ${params.GenesisURL}"
      } catch(Exception e) {
        error("Failed to download genesis file from URL: ${params.GenesisURL}. Valid genesis URL must be provided for Custom networks!")
      }

    } else {
        error("Invalid \"Network\" option selected...")
    }
}

def start_build() {
  sh "cmake . -DUSE_PCH=OFF \
              -DZLIB_ROOT=${ZLIB} \
              -DBOOST_ROOT=${BOOST_167} \
              -DOPENSSL_ROOT_DIR=${OPENSSL_111} \
              -DSQLITE3_ROOT_DIR=${SQLITE_3253} \
              -DSOPHIATX_STATIC_BUILD=ON \
              -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
              -DCMAKE_INSTALL_PREFIX=install \
              -DSOPHIATX_EGENESIS_JSON=${GENESIS_FILE} \
              -DBUILD_SOPHIATX_TESTNET=${BUILD_TESTNET} \
              -DAPP_INSTALL_DIR=install/bin/ \
              -DCONF_INSTALL_DIR=install/etc \
              -DSERVICE_INSTALL_DIR=install/etc"

  sh 'make install -j4'
}

def tests() {
  script {
    if( BUILD_TESTNET == "false" ) {
      sh './tests/chain_test'
      sh './tests/plugin_test'
      //sh './tests/smart_contracts/smart_contracts_tests'
      sh './tests/utilities/utilities_tests'
      sh './libraries/fc/vendor/secp256k1-zkp/src/project_secp256k1-build/tests'
      sh './libraries/fc/tests/all_tests'
      //sh './libraries/SQLiteCpp/SQLiteCpp_tests'
    }
  }
}

def run_archive() {
  dir('install') {
    dir('lib') {
      script {
        echo "${LIB_ARCHIVE_NAME}"
        if( !params.Debug ) {
          try {
              sh 'strip -s libalexandria.so libalexandriaJNI.so *_plugin.so' //strip symbols
              } catch(Exception e) {
                echo "Skipping strip"
              }
            }
          }
      sh "tar -czf ${LIB_ARCHIVE_NAME} libalexandria.so libalexandriaJNI.so alexandria.hpp AlexandriaJNI.java" //create tar file
      sh "tar -czf ${PLUGIN_ARCHIVE_NAME} *_plugin.[^a]*" //create tar file
      archiveArtifacts '*.gz'
    }
  dir('bin') {
    sh 'rm -f test*' //remove test binaries
    script {
      echo "${ARCHIVE_NAME}"
      if( !params.Debug ) {
        try {
            sh 'strip -s *' //strip symbols
            } catch(Exception e) {
              echo "Skipping strip"
            }
          }

          if( BUILD_TESTNET == "true" ) {
           sh "cp ${WORKSPACE}/contrib/testnet_config.ini ."//copy config
           sh "tar -czf ${ARCHIVE_NAME} cli_wallet sophiatxd sophiatxd_light testnet_config.ini" //create tar file
           } else {
           sh "cp ${WORKSPACE}/contrib/fullnode_config.ini ."//copy configs
           sh "cp ${WORKSPACE}/contrib/witness_config.ini ."//copy configs
           sh "tar -czf ${ARCHIVE_NAME} cli_wallet sophiatxd sophiatxd_light fullnode_config.ini witness_config.ini/" //create tar file
         }
       }
       archiveArtifacts '*.gz'
     }
   }
 }

 def create_packages() {
    if (params.Package == "") {
        return
    }

    // If there is existing cmakecache from previous build, delete it as we want
    // different output directory for build files
    if (fileExists('CMakeCache.txt') == true) {
        sh "rm -f CMakeCache.txt"
    }

    if (params.Package.contains("sophiatx")) {
        build_package("programs/sophiatxd")
    }

    if (params.Package.contains("light-client")) {
        build_package("programs/sophiatxd_light")
    }

    if (params.Package.contains("cli-wallet")) {
        build_package("programs/cli_wallet")
    }
 }

 def build_package(String dirPath) {
    dir(dirPath) {
        dir("package") {
            sh 'export DEB_BUILD_OPTIONS="parallel=4"'
            sh "debuild --set-envvar CMAKE_BUILD_TYPE_ENV=${BUILD_TYPE} \
                        --set-envvar BUILD_SOPHIATX_TESTNET_ENV=${BUILD_TESTNET} \
                        --set-envvar SOPHIATX_EGENESIS_JSON_ENV=${GENESIS_FILE} \
                        --set-envvar OPENSSL_ROOT_DIR_ENV=${OPENSSL_111} \
                        --set-envvar BOOST_ROOT_DIR_ENV=${BOOST_167} \
                        -uc -us"
        }

        archiveArtifacts '*.deb'
    }
 }

 def checkBox (String name, String values, String defaultValue,
               int visibleItemCnt=0, String type, String description='', String delimiter=',') {

     // default same as number of values
     visibleItemCnt = visibleItemCnt ?: values.split(',').size()
     return new ExtendedChoiceParameterDefinition(
             name, //name,
             type, //type
             values, //value
             "", //projectName
             "", //propertyFile
             "", //groovyScript
             "", //groovyScriptFile
             "", //bindings
             "", //groovyClasspath
             "", //propertyKey
             defaultValue, //defaultValue
             "", //defaultPropertyFile
             "", //defaultGroovyScript
             "", //defaultGroovyScriptFile
             "", //defaultBindings
             "", //defaultGroovyClasspath
             "", //defaultPropertyKey
             "", //descriptionPropertyValue
             "", //descriptionPropertyFile
             "", //descriptionGroovyScript
             "", //descriptionGroovyScriptFile
             "", //descriptionBindings
             "", //descriptionGroovyClasspath
             "", //descriptionPropertyKey
             "", //javascriptFile
             "", //javascript
             false, //saveJSONParameterToFile
             false, //quoteValue
             visibleItemCnt, //visibleItemCount
             description, //description
             delimiter //multiSelectDelimiter
             )
 }