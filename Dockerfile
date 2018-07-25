FROM phusion/baseimage:0.9.19

#ARG SOPHIATXD_BLOCKCHAIN=https://example.com/sophiatxd-blockchain.tbz2

ARG SOPHIATX_STATIC_BUILD=ON
ENV SOPHIATX_STATIC_BUILD ${SOPHIATX_STATIC_BUILD}

ENV LANG=en_US.UTF-8

RUN \
    apt-get update && \
    apt-get install -y \
        autoconf \
        automake \
        autotools-dev \
        bsdmainutils \
        build-essential \
        cmake \
        doxygen \
        gdb \
        git \
        libboost-all-dev \
        libreadline-dev \
        libssl-dev \
        libtool \
        liblz4-tool \
        ncurses-dev \
        pkg-config \
        python3 \
        python3-dev \
        python3-jinja2 \
        python3-pip \
        nginx \
        fcgiwrap \
        awscli \
        jq \
        wget \
        virtualenv \
    && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/* && \
    pip3 install gcovr

ADD . /usr/local/src/sophiatx

RUN \
    cd /usr/local/src/sophiatx && \
    git submodule update --init --recursive && \
    mkdir build && \
    cd build && \
    cmake \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_SOPHIATX_TESTNET=ON \
        -DLOW_MEMORY_NODE=OFF \
        -DCLEAR_VOTES=ON \
        -DSKIP_BY_TX_ID=ON \
        .. && \
    make -j$(nproc) chain_test test_fixed_string plugin_test && \
    ./tests/chain_test && \
    ./tests/plugin_test && \
    ./programs/util/test_fixed_string && \
    cd /usr/local/src/sophiatx && \
    doxygen && \
    programs/build_helpers/check_reflect.py && \
    programs/build_helpers/get_config_check.sh && \
    rm -rf /usr/local/src/sophiatx/build

RUN \
    cd /usr/local/src/sophiatx && \
    git submodule update --init --recursive && \
    mkdir build && \
    cd build && \
    cmake \
        -DCMAKE_INSTALL_PREFIX=/usr/local/sophiatxd-testnet \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_SOPHIATX_TESTNET=ON \
        -DLOW_MEMORY_NODE=OFF \
        -DCLEAR_VOTES=ON \
        -DSKIP_BY_TX_ID=ON \
        -DENABLE_SMT_SUPPORT=ON \
        -DSOPHIATX_STATIC_BUILD=${SOPHIATX_STATIC_BUILD} \
        .. && \
    make -j$(nproc) chain_test test_fixed_string plugin_test && \
    make install && \
    ./tests/chain_test && \
    ./tests/plugin_test && \
    ./programs/util/test_fixed_string && \
    cd /usr/local/src/sophiatx && \
    doxygen && \
    programs/build_helpers/check_reflect.py && \
    programs/build_helpers/get_config_check.sh && \
    rm -rf /usr/local/src/sophiatx/build

RUN \
    cd /usr/local/src/sophiatx && \
    git submodule update --init --recursive && \
    mkdir build && \
    cd build && \
    cmake \
        -DCMAKE_BUILD_TYPE=Debug \
        -DENABLE_COVERAGE_TESTING=ON \
        -DBUILD_SOPHIATX_TESTNET=ON \
        -DLOW_MEMORY_NODE=OFF \
        -DCLEAR_VOTES=ON \
        -DSKIP_BY_TX_ID=ON \
        -DCHAINBASE_CHECK_LOCKING=OFF \
        .. && \
    make -j$(nproc) chain_test plugin_test && \
    ./tests/chain_test && \
    ./tests/plugin_test && \
    mkdir -p /var/cobertura && \
    gcovr --object-directory="../" --root=../ --xml-pretty --gcov-exclude=".*tests.*" --gcov-exclude=".*fc.*" --gcov-exclude=".*app*" --gcov-exclude=".*net*" --gcov-exclude=".*plugins*" --gcov-exclude=".*schema*" --gcov-exclude=".*time*" --gcov-exclude=".*utilities*" --gcov-exclude=".*wallet*" --gcov-exclude=".*programs*" --output="/var/cobertura/coverage.xml" && \
    cd /usr/local/src/sophiatx && \
    rm -rf /usr/local/src/sophiatx/build

RUN \
    cd /usr/local/src/sophiatx && \
    git submodule update --init --recursive && \
    mkdir build && \
    cd build && \
    cmake \
        -DCMAKE_INSTALL_PREFIX=/usr/local/sophiatxd-default \
        -DCMAKE_BUILD_TYPE=Release \
        -DLOW_MEMORY_NODE=ON \
        -DCLEAR_VOTES=ON \
        -DSKIP_BY_TX_ID=OFF \
        -DBUILD_SOPHIATX_TESTNET=OFF \
        -DSOPHIATX_STATIC_BUILD=${SOPHIATX_STATIC_BUILD} \
        .. \
    && \
    make -j$(nproc) && \
    make install && \
    cd .. && \
    ( /usr/local/sophiatxd-default/bin/sophiatxd --version \
      | grep -o '[0-9]*\.[0-9]*\.[0-9]*' \
      && echo '_' \
      && git rev-parse --short HEAD ) \
      | sed -e ':a' -e 'N' -e '$!ba' -e 's/\n//g' \
      > /etc/sophiatxdversion && \
    cat /etc/sophiatxdversion && \
    rm -rfv build && \
    mkdir build && \
    cd build && \
    cmake \
        -DCMAKE_INSTALL_PREFIX=/usr/local/sophiatxd-full \
        -DCMAKE_BUILD_TYPE=Release \
        -DLOW_MEMORY_NODE=OFF \
        -DCLEAR_VOTES=OFF \
        -DSKIP_BY_TX_ID=ON \
        -DBUILD_SOPHIATX_TESTNET=OFF \
        -DSOPHIATX_STATIC_BUILD=${SOPHIATX_STATIC_BUILD} \
        .. \
    && \
    make -j$(nproc) && \
    make install && \
    rm -rf /usr/local/src/sophiatx

RUN \
    apt-get remove -y \
        automake \
        autotools-dev \
        bsdmainutils \
        build-essential \
        cmake \
        doxygen \
        dpkg-dev \
        libboost-all-dev \
        libc6-dev \
        libexpat1-dev \
        libgcc-5-dev \
        libhwloc-dev \
        libibverbs-dev \
        libicu-dev \
        libltdl-dev \
        libncurses5-dev \
        libnuma-dev \
        libopenmpi-dev \
        libpython-dev \
        libpython2.7-dev \
        libreadline-dev \
        libreadline6-dev \
        libssl-dev \
        libstdc++-5-dev \
        libtinfo-dev \
        libtool \
        linux-libc-dev \
        m4 \
        make \
        manpages \
        manpages-dev \
        mpi-default-dev \
        python-dev \
        python2.7-dev \
        python3-dev \
    && \
    apt-get autoremove -y && \
    rm -rf \
        /var/lib/apt/lists/* \
        /tmp/* \
        /var/tmp/* \
        /var/cache/* \
        /usr/include \
        /usr/local/include

RUN useradd -s /bin/bash -m -d /var/lib/sophiatxd sophiatxd

RUN mkdir /var/cache/sophiatxd && \
    chown sophiatxd:sophiatxd -R /var/cache/sophiatxd

# add blockchain cache to image
#ADD $SOPHIATXD_BLOCKCHAIN /var/cache/sophiatxd/blocks.tbz2

ENV HOME /var/lib/sophiatxd
RUN chown sophiatxd:sophiatxd -R /var/lib/sophiatxd

VOLUME ["/var/lib/sophiatxd"]

# rpc service:
EXPOSE 8090
# p2p service:
EXPOSE 2001

# add seednodes from documentation to image
ADD doc/seednodes.txt /etc/sophiatxd/seednodes.txt

# the following adds lots of logging info to stdout
ADD contrib/config-for-docker.ini /etc/sophiatxd/config.ini
ADD contrib/fullnode.config.ini /etc/sophiatxd/fullnode.config.ini
ADD contrib/fullnode.opswhitelist.config.ini /etc/sophiatxd/fullnode.opswhitelist.config.ini
ADD contrib/config-for-broadcaster.ini /etc/sophiatxd/config-for-broadcaster.ini
ADD contrib/config-for-ahnode.ini /etc/sophiatxd/config-for-ahnode.ini
ADD contrib/testnet.config.ini /etc/sophiatxd/testnet.config.ini
ADD contrib/fastgen.config.ini /etc/sophiatxd/fastgen.config.ini

# add normal startup script that starts via sv
ADD contrib/sophiatxd.run /usr/local/bin/sophiatx-sv-run.sh
RUN chmod +x /usr/local/bin/sophiatx-sv-run.sh

# add nginx templates
ADD contrib/sophiatxd.nginx.conf /etc/nginx/sophiatxd.nginx.conf
ADD contrib/healthcheck.conf.template /etc/nginx/healthcheck.conf.template

# add PaaS startup script and service script
ADD contrib/startpaassophiatxd.sh /usr/local/bin/startpaassophiatxd.sh
ADD contrib/testnetinit.sh /usr/local/bin/testnetinit.sh
ADD contrib/paas-sv-run.sh /usr/local/bin/paas-sv-run.sh
ADD contrib/sync-sv-run.sh /usr/local/bin/sync-sv-run.sh
ADD contrib/healthcheck.sh /usr/local/bin/healthcheck.sh
RUN chmod +x /usr/local/bin/startpaassophiatxd.sh
RUN chmod +x /usr/local/bin/testnetinit.sh
RUN chmod +x /usr/local/bin/paas-sv-run.sh
RUN chmod +x /usr/local/bin/sync-sv-run.sh
RUN chmod +x /usr/local/bin/healthcheck.sh

# new entrypoint for all instances
# this enables exitting of the container when the writer node dies
# for PaaS mode (elasticbeanstalk, etc)
# AWS EB Docker requires a non-daemonized entrypoint
ADD contrib/sophiatxdentrypoint.sh /usr/local/bin/sophiatxdentrypoint.sh
RUN chmod +x /usr/local/bin/sophiatxdentrypoint.sh
CMD /usr/local/bin/sophiatxdentrypoint.sh
