# Building SophiaTX

## Compile-Time Options (cmake)

### CMAKE_BUILD_TYPE=[Release/Debug]

Specifies whether to build with or without optimization and without or with
the symbol table for debugging. Unless you are specifically debugging or
running tests, it is recommended to build as release.

### LOW_MEMORY_NODE=[OFF/ON]

Builds sophiatxd to be a consensus-only low memory node. Data and fields not
needed for consensus are not stored in the object database.  This option is
recommended for witnesses and seed-nodes.

### CLEAR_VOTES=[ON/OFF]

Clears old votes from memory that are no longer required for consensus.

### BUILD_SOPHIATX_TESTNET=[OFF/ON]

Builds sophiatx for use in a private testnet. Also required for building unit tests.

### SKIP_BY_TX_ID=[OFF/ON]

By default this is off. Enabling will prevent the account history plugin querying transactions 
by id, but saving around 65% of CPU time when reindexing. Enabling this option is a
huge gain if you do not need this functionality.

## Building on Ubuntu 18.04.1 
For Ubuntu 18.04.1 users, after installing the right packages with `apt` SophiaTX
will build out of the box without further effort:
    
    # Required packages
    sudo apt install -y \
        autoconf \
        automake \
        cmake \
        gcc \
        g++ \
        git \
        libbz2-dev \
        libssl-dev \
        libtool \
        make \
        pkg-config \
        python3 \
        python3-jinja2 \
        zlib1g-dev
        
    # Boost packages (also required)
    sudo apt-get install -y \
        libboost-chrono-dev \
        libboost-context-dev \
        libboost-coroutine-dev \
        libboost-date-time-dev \
        libboost-filesystem-dev \
        libboost-iostreams-dev \
        libboost-locale-dev \
        libboost-program-options-dev \
        libboost-serialization-dev \
        libboost-signals-dev \
        libboost-system-dev \
        libboost-test-dev \
        libboost-thread-dev

    # Optional packages (not required, but will make a nicer experience)
    sudo apt install -y \
        doxygen \
        libncurses5-dev \
        libreadline-dev \
        perl
        
    git clone https://github.com/SophiaTX/SophiaTX
    cd SophiaTX
    git checkout master
    git submodule update --init --recursive
    mkdir build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release ..
    make -j$(nproc) sophiatxd
    make -j$(nproc) cli_wallet
    
    # optional
    make install  # defaults to /usr/local

## Building Boost 1.67

SophiaTX requires Boost 1.65 and works with versions up to 1.69 (including).
Here is how to build and install Boost 1.67 into your user's home directory

    export BOOST_ROOT=$HOME/opt/boost_1_67_0
    URL='http://sourceforge.net/projects/boost/files/boost/1.67.0/boost_1_67_0.tar.gz'
    wget "$URL"
    [ $( sha256sum boost_1_67_0.tar.gz | cut -d ' ' -f 1 ) == \
        "8aa4e330c870ef50a896634c931adf468b21f8a69b77007e45c444151229f665" ] \
        || ( echo 'Corrupt download' ; exit 1 )
    tar xzf boost_1_67_0.tar.gz
    cd boost_1_67_0
    ./bootstrap.sh "--prefix=$BOOST_ROOT"
    ./b2 -j$(nproc) install

## Building on macOS X

Install Xcode and its command line tools by following the instructions here:
https://guide.macports.org/#installing.xcode. In OS X 10.11 (El Capitan)
and newer, you will be prompted to install developer tools when running a
developer command in the terminal.

Accept the Xcode license if you have not already:

    sudo xcodebuild -license accept

Install Homebrew by following the instructions here: http://brew.sh/

### Initialize Homebrew:

    brew doctor
    brew update

### Install sophiatx dependencies:

    brew install \
        autoconf \
        automake \
        cmake \
        git \
        boost \
        libtool \
        openssl \
        python3
        
    pip3 install --user jinja2
    
*Optional.* To use TCMalloc in LevelDB:

    brew install google-perftools

*Optional.* To use cli_wallet and override macOS's default readline installation:

    brew install --force readline
    brew link --force readline

### Clone the Repository

    git clone https://github.com/SophiaTX/SophiaTX
    cd sophiatx

### Compile

    export OPENSSL_ROOT_DIR=$(brew --prefix)/Cellar/openssl/1.1/
    export BOOST_ROOT=$(brew --prefix)/Cellar/boost@1.67/1.67.0_1/
    git checkout master
    git submodule update --init --recursive
    mkdir build && cd build
    cmake -DBOOST_ROOT="$BOOST_ROOT" -DCMAKE_BUILD_TYPE=Release ..
    make -j$(sysctl -n hw.logicalcpu)

Please note, that actual version of `openssl` and `boost` might differ.
Also, some useful build targets for `make` are:

    sophiatxd
    chain_test
    cli_wallet

e.g.:

    make -j$(sysctl -n hw.logicalcpu) sophiatxd

This will only build `sophiatxd`.

## Building on Other Platforms

- Windows build instructions are available here https://github.com/SophiaTX/SophiaTX/wiki/Setting-up-Windows-build-enviroment

- The developers normally compile with gcc and clang. These compilers should
  be well-supported.
- Community members occasionally attempt to compile the code with mingw,
  Intel and Microsoft compilers. These compilers may work, but the
  developers do not use them. Pull requests fixing warnings / errors from
  these compilers are accepted.
