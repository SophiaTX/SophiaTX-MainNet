#!/bin/bash

echo /tmp/core | tee /proc/sys/kernel/core_pattern
ulimit -c unlimited

# if we're not using PaaS mode then start sophiatxd traditionally with sv to control it
if [[ ! "$USE_PAAS" ]]; then
  mkdir -p /etc/service/sophiatxd
  cp /usr/local/bin/sophiatx-sv-run.sh /etc/service/sophiatxd/run
  chmod +x /etc/service/sophiatxd/run
  runsv /etc/service/sophiatxd
elif [[ "$IS_TESTNET" ]]; then
  /usr/local/bin/testnetinit.sh
else
  /usr/local/bin/startpaassophiatxd.sh
fi
