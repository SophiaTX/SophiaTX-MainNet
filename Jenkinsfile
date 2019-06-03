#!groovy

import com.cwctravel.hudson.plugins.extended_choice_parameter.ExtendedChoiceParameterDefinition

////////////////////////////////////////

properties([parameters([booleanParam(defaultValue: false, description: 'Build in debug mode', name: 'Debug'),
                        string(defaultValue: """, description: 'Custom genesis URL(valid only for Customnet)', name: 'CustomGenesisUrl'),
                        checkBox("Network", "Mainnet,Testnet,Customnet", "Testnet" /*default*/, 0, "PT_SINGLE_SELECT", "Select network"),
                        checkBox("Package", "sophiatx,sophiatx-light,cli-wallet", "" /*default*/, 0, "PT_CHECKBOX", "Select packages to be built")
                      ])
          ])


pipeline {
  options {
    buildDiscarder(logRotator(artifactNumToKeepStr: '5'))
    skipDefaultCheckout()
    parallelsAlwaysFailFast() 
  }
  environment {
    GENESIS_FILE = "genesis.json"
    BUILD_TESTNET = "false"
    BUILD_TYPE = "Release"
  }
  agent any
  stages {
      stage('Input') {
            steps {
                input('Do you want to proceed?')
            }
        }

        stage('If Proceed is clicked') {
            steps {
                print('hello')
            }
        }
    stage('Init build variables') {
      steps {
        echo "Network: ${params.Network}"
        echo "Package: ${params.Package}"
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
    }

    if( params.Network == "Mainnet" ) {
      BUILD_TESTNET = "false"
      GENESIS_FILE = "genesis.json"
    }
    else if( params.Network == "Testnet" ) {
      BUILD_TESTNET = "true"
      GENESIS_FILE = "genesis_testnet.json"
    }
    else {
      BUILD_TESTNET = "false"

      def customGenesis
      // Get the input
      def userInput = input(
              id: 'userInput', message: 'Enter url of custom genesis:?',
              parameters: [
                      string(defaultValue: 'None',
                              description: 'Url of custom genesis file',
                              name: 'CustomGenesisUrl')
              ])

      // Save to variables. Default to empty string if not found.
      customGenesis = userInput.CustomGenesisUrl?:''

      // Echo to console
      echo("Custom Genesis URL: ${customGenesis}")

      // Wget
      // ...
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
           sh "cp -r ${WORKSPACE}/etc ."
           sh "tar -czf ${ARCHIVE_NAME} cli_wallet sophiatxd sophiatxd_light testnet_config.ini" //create tar file
           } else {
           sh "cp ${WORKSPACE}/contrib/fullnode_config.ini ."//copy configs
           sh "cp ${WORKSPACE}/contrib/witness_config.ini ."//copy configs
           sh "cp -r ${WORKSPACE}/etc ."
           sh "tar -czf ${ARCHIVE_NAME} cli_wallet sophiatxd sophiatxd_light fullnode_config.ini witness_config.ini/" //create tar file
         }
       }
       archiveArtifacts '*.gz'
     }
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