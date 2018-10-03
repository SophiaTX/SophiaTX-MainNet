#!/bin/bash

while getopts "h:u:s:r" opt; do
  case $opt in
    h)
      host="$OPTARG"
      ;;
    u)
      user="$OPTARG"
      ;;
    s)
      sourceUrl="$OPTARG"
      ;;
    r)
      replayBlockchain=true
      ;;
    *)
  esac
done


# check required variables
usageMessage="$0 -h host -u user [-s sourceUrl] [-r]"
[[ $# -eq 0 || -z $host ]] && { echo "-h host parameter is required ! Usage: \"$usageMessage\""; exit 1; }
[[ $# -eq 0 || -z $user ]] && { echo "-u user parameter is required ! Usage: \"$usageMessage\""; exit 1; }

# set default values for unset optional pars
if [[ -z $sourceUrl ]]
then
  sourceUrl="https://jenkins.sophiatx.com/job/SophiaTX-develop/lastSuccessfulBuild/artifact/*zip*/archive.zip"
fi

if [[ -z $replayBlockchain ]]
then
  replayBlockchain=false
fi

ansible-playbook ansible/playbooks/update-node.yml --extra-vars "host=$host user=$user sourceUrl=$sourceUrl replayBlockchain=$replayBlockchain" -i $host,

if [ $? -ne 0 ]; then
  echo "ERROR while updating node on \"${host}\". Error code: $?"
  exit 2
fi
