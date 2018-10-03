# Automatic node deploy

Script connects to the server and automatically deploys node binaries and stores previous version

Created file structure on server:
 
     ~
     ├── ...                        
     ├── sophiatx-binaries                  # Binaries folder
     │   ├── sophiatx_#<NUM>.tar.gz         # SophiaTX archived binaries
     │   ├── sophiatx_#<NUM>.tar.gz.old     # Previous version of SophiaTX archived binaries
     │   ├── testnet_config.ini             # SophiaTX demon config file
     │   ├── testnet_config.ini.old         # Previous version of SophiaTX demon config file
     │   ├── sophiatxd                      # SophiaTX demon
     │   ├── alexandria_deamon              # Alexandria demon
     │   └── bc-data                        # Blokchain data folder
     │       ├── config.ini                 # Copied testnet_config.ini
     │       └── ...
     └── ... 
    

## Usage

./deploy-node.sh -h host -u user [-s sourceUrl] [-r]

####Arguments description:

-h host         : host(server), where node binaries should be deployed into
  
-u user         : user to be used to connect to the host. This user has to have id_rsa.pub imported on host
  
[-s sourceUrl]  : sourceUrl from which are the the binaries downloaded. optional. In case it is not specified, latest binaries are downloaded.
  
[-r]            : replay-blockain flag. It is used when starting sophiatxd. optional 


## Requirements

For using automatic deploy script, you have to install Ansible on your machine:

$ sudo apt-get update   
$ sudo apt-get install software-properties-common   
$ sudo apt-add-repository ppa:ansible/ansible   
$ sudo apt-get update   
$ sudo apt-get install ansible