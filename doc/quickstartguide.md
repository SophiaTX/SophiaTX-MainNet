Quickstart Guide
-------------------

### Pre-requisites

* A sufficient-sized and secure machine (Physical, VM or in the Cloud) with any NIX system (SUSE, openSUSE, Redhat, Ubuntu, Debian,etc.) pre-installed on it
* System requirements:  
  * CPU:  min. 2 cores 
  * RAM: min. 4 GB 
  * Disk space –  > 50GB (blockchain and OS)
  * Disk Partitions  
    Recommended to create dedicated partition for storing blockchain database (data); sizing to be adjusted according to the size of the actual blockchain (e.g. 50 GB could be sufficient for 1 year at the time when SophiaTX MainNet was launched) 

    for other partitions, setup and sizing, follow the best practice for NIX system installation 

  * Network – the network should be reliable and fast (high speed connection with low latency), and the machine accessible via public IP 
  * 2 x 1Gb >= Network adapter (second Network Interface Card (NIC) as a standby or failover interface in case the primary NIC fails. Can also be used for  load balancing)  

  * The following are the port(s) to be opened for incoming traffic or connections: 
  #### MANDATORY PORT 
  60000 (default port nr. for P2P endpoint blockchain nodes communication) 
  
  `p2p-endpoint = 0.0.0.0:60000`

  #### OPTIONAL PORTS 
  9193 (default port nr. for Local http endpoint for webserver requests)  

  `webserver-http-endpoint = a.b.c.d:9193`

   9191 (default port for Local web-socket endpoint for webserver requests) 

   ` webserver-ws-endpoint = a.b.c.d:9191`

    There is no need to keep these 2 ports (http & ws) open on the witness node. 

    For any testing purposes on the witness node, restrict the usage of http & ws ports for a limited time period and only to locahost = 127.0.0.1 or to a dedicated host IP – a.b.c.d. The setting to 0.0.0.0 is not recommended and to be avoided. Restrict accepting incoming http or ws requests to only specific IP or net. For net - define network as a.b.0.0 / for dedicated IP as a.b.c.d. 

    All ports are configurable and can be subject to adjustment, either in the config.ini file or added as a parameter(s) upon the starting of sophiatxd deamon.

    Example of configuration of the network parameters (IP & Ports) in config.ini:

    #### The local IP address and port to listen for incoming connections.  

    `p2p-endpoint = 154.32.0.0:61234 `

    #### Local http endpoint for webserver requests. 

    `webserver-http-endpoint = 127.0.0.1:1193`

    #### Local websocket endpoint for webserver requests. 

    `webserver-ws-endpoint = 127.0.0.1:2291` 

    Example to start sophiatxd with adjusted network parameters: 

    ```./sophiatxd -d /sphtxBCdata --p2p-endpoint 154.32.0.0:61234 --webserver-http-endpoint 127.0.0.1:1193 --webserver-ws-endpoint 127.0.0.1:2291``` 

### Dedicated Operating System account to run blockchain applications 
  There is no urgency to install and run blockchain applications (daemon, wallet, …) under privileged account as a root. 

  In general, as your own user account, programs you run are restricted from writing to the rest of the system – they can only write to your “home” folder. You cannot modify system files without gaining root permissions first. This helps keep your computer secure. 

  To fulfil this requirement, it is recommended to create a dedicated user and group on NIX system

    * Group: sphtxgrp  
    * Account: sphtxamd  

  __!!! You must gain sufficient privileges (e.g. as a root user or you can also prefix the following commands with the sudo command to run them as a non-privileged user to create a group and an account!!!__

  CLI commands to add group and account. The basic syntax of the command to Create/Add a new Group. As a root user you will type and run this command from command line.
  
  `# groupadd sphtxgrp `     
  
  or as a john user you will type and run this command from command line. 
  
  `john@EUGREEN02:~> sudo groupadd sphtxgrp`
  
  In the next steps follow the same principle for running the commands either as  a root user or as any other OS user by using  the sudo command 
  The basic syntax of the command to Create/Add a new User with Specific Home Directory, Default Shell and Custom Comment from command line: 
  
  `# useradd -m -g sphtxgrp -d /home/sphtxadm -s /bin/bash -c "SophiaTX Blockchain Owner" sphtxadm` 
  
  To set password for user sphtxamd 
  
  `# passwd sphtxadm `
  
  There are many other possibilities how to create users and groups in the NIX system – just follow NIX documentation / best praxis.  
  
## Install SophiaTX blockchain  

To store blockchain data, it is suggested to create and have mounted a supplemental dedicated partition/file system. Subject of pre-requisites. Create subdirectory sphtxBCdata in the / directory and adjust ownership & permissions to user sphtxadm.
```
# mkdir /sphtxBCdata 
# chown sphtxadm:sphtxgrp /sphtxBCdata 
```
Make sure that dedicated partition and file system for blockdata is mounted under previously created directory sphtxBCdata. Don’t forget to adjust /etc/fstab accordingly in order to avoid problems after rebooting the OS.  
To validate that a dedicated partition is mounted under /sphtxBCdata, launch the  mount command. 
```
# mount 
sysfs on /sys type sysfs (rw,nosuid,nodev,noexec,relatime) 
proc on /proc type proc (rw,nosuid,nodev,noexec,relatime) 
devtmpfs on /dev type devtmpfs (rw,nosuid,size=2011880k,nr_inodes=502970,mode=755) 
securityfs on /sys/kernel/security type securityfs (rw,nosuid,nodev,noexec,relatime) 
tmpfs on /dev/shm type tmpfs (rw,nosuid,nodev,size=8388608k) 
devpts on /dev/pts type devpts (rw,nosuid,noexec,relatime,gid=5,mode=620,ptmxmode=000) 
tmpfs on /run type tmpfs (rw,nosuid,nodev,mode=755) 
/dev/sda2 on /sphtxBCdata type xfs (rw,relatime,attr2,inode64,noquota) 
```

If no record about /sphtxBCdata is found in the output, then you have to mount the partition as follows: 
In case there exists sdb2 partition issue the following command from command line 

`# mount /dev/sdb2 /sphtxBCdata `

There are many other options on how to check and mount the file system under specific directory and make sure it will be mounted automatically after the reboot. Just follow NIX documentation / best praxis.  

The following commands to be executed as a sphtxadm user. You may log in as sphtxadm user, or switch to sphtxadm user by issuing of the following command: 

`# su - sphtxadm`    

Create and change to subdirectory sphtxd in home directory of sphtxadm user 

```
sphtxadm@EUGREEN02:~> mkdir sphtxd 
sphtxadm@EUGREEN02:~> cd sphtxd 
```
Go & Download the latest version of sophiatx binary and config files from the GitHub repository https://github.com/SophiaTX/SophiaTX/releases 

#### Unpack downloaded sophiatx.tar.gz file 
```
sphtxadm@EUGREEN02:~/sphtxd> tar -xvf sophiatx.tar.gz 
alexandria_deamon 
cli_wallet 
sophiatxd 
witness_config.ini 
fullnode_config.ini
```
If required, adjust witness_config.ini and get it copied as config.ini into the blockchain data directory. Make sure that ownership and privileges of the config file are set accordingly.  

```
sphtxadm@EUGREEN02:~/sphtxd>  chmod 600 ./witness_config.ini 
sphtxadm@EUGREEN02:~/sphtxd>  chown sphtxadm:sphtxgrp ./witness_config.ini 
sphtxadm@EUGREEN02:~/sphtxd>  ls -l ./witness_config.ini 
-rw------- 1 sphatxdm sphtxgrp     5671 Jul 25 17:26 witness_config.ini 
```
There might be a few reasons to update the config.ini file. Adjust witness private key and name of witness controlled by node 
```
witness = “<name of witness>” 
private-key = <WIF Private Key>
```
Example 

```
witness = "I889UHminer" 
private-key = 5IJY3b9FgfsGtxMeLkLqXzUrQDMAsqSyAZDnMBkg7PDDRhQgaV 
```
Adjust a list of plugins 
#### Mandatory plugin(s) – required to run witness node 

`plugin = witness` 

#### Optional plugin(s) – NOT required to run witness node 

```
#plugin = block_api 
#plugin = network_broadcast_api 
#plugin = database_api 
#plugin = condenser_api 
#plugin = account_history 
#plugin = account_history_api 
#plugin = account_by_key 
#plugin = chain_api 
#plugin = custom_api 
```
Adjust a default port number for P2P endpoint blockchain nodes communication

`p2p-endpoint = 0.0.0.0:60000` 

Make sure that "enable-stale-production" is set on true  

`enable-stale-production = true`

Copy witness_config.ini into config.ini in the blockchain data directory

`sphtxadm@EUGREEN02:~/sphtxd>  cp ./witness_config.ini /sphtxBCdata/config.ini` 


__To run SophiaTX Witness node and produce blocks (meaning being eligible to validate the transactions embedded in the  blocks) you need to be one of the elected SophiaTX witnesses__

#### Run SophiaTX Witness 

There are two options to launch and run the SophiaTX witness node. When all mandatory parameters have been embedded in config.ini file, you may run the witness node with the following command: 

 `sophiatxd -d <data_dir>` 

Example 

`sphtxadm@EUGREEN02:~/sphtxd>  ./sophiatxd -d /sphtxBCdata`

Parameters to be entered from command line. 

```sophiatxd -d <data_dir> --enable-stale-production true –-witness ‘<witness_name>’ --private-key <WIF Private Key> --plugin witness```  

Example: 

```sphtxadm@EUGREEN02:~/sphtxd>  ./sophiatxd -d /sphtxBCdata --enable-stale-production true --witness '" I889UHminer"' --private-key 5JPwY3bwFgfsGtxMeLkLqXzUrQDMAsqSyAZDnMBkg7PDDRhQgaV --plugin witness``` 

 
