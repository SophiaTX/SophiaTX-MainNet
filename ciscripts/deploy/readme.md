# Automatic node deploy

Script connects to the server and automatically deploys node binaries

## Usage

./updare-node.sh -h host -u user [-s sourceUrl] [-r]

####Arguments description:

-h host         : host of the server to be updated
  
-u user         : user to be used to connect to the host. This user has to have id_rsa.pub imported on host
  
[-s sourceUrl]  : sourceUrl from which are the the binaries downloaded. optional. In case it is not specified, latest binaries are downloaded.
  
[-r]            : replay-blockain. Flag used when starting sophiatxd. optional 


## Requirements

For using automatic deploy script, you have to install Ansible on your machine:

$ sudo apt-get update   
$ sudo apt-get install software-properties-common   
$ sudo apt-add-repository ppa:ansible/ansible   
$ sudo apt-get update   
$ sudo apt-get install ansible