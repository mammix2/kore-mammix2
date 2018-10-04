#!/bin/sh

if [ $# -lt 5 ]
then
echo "#############################################################################"
echo "#############################################################################"
echo "#############################################################################"
echo "## "
echo "## This is a Kore script to help create a masternode configuration file"
echo "## create_masternode_step1 <network> <name> <onion-address> <user> <password>"
echo "## Parameters"
echo "##   network: mainnet or testnet"
echo "##   name: a name for your masternode"
echo "##   onion-address: your masternode onion address"
echo "##   user: user name to use in the masternode configuration"
echo "##   password: user's password for masternode configuration"
echo "## example:"
echo "##   create_masternode_step1.sh testnet MasternodeI 443xtqzgm2vxi7fy.onion  kore 12345"
echo "## "
echo "## PLEASE install jq from here https://stedolan.github.io/jq/"
echo "#############################################################################"
echo "#############################################################################"

exit 1
fi

echo "##########################################################################"
echo "####################                          ############################"
echo "#################### MASTERNODE CONFIGURATION ############################"
echo "####################                          ############################"

dir=`pwd`
masternode_coins_amount=500
network=$1
masternode_name=$2
masternode_onion_address=$3
masternode_user=$4
masternode_password=$5
control_wallet_user=kore
control_wallet_password=kore
masternode_conf_file="$dir/$masternode_name.conf"
control_wallet="$dir/masternode.conf.$masternode_name"

cli_args="-$network -debug -rpcuser=$control_wallet_user -rpcpassword=$control_wallet_password"
if [ "$network" = "testnet" ] || [ "$network" = "TESTNET" ]
then
  masternode_port=11743
else
  masternode_port=10743
fi

echo "## "
echo "## Parameters used"
echo "##   network: $network"
echo "##   masternode-name   : $masternode_name"
echo "##   masternode-address: $masternode_onion_address"
echo "##   masternode-port   : $masternode_port"
echo "##   user    : $masternode_user"
echo "##   password: $masternode_password"
echo "##   cli args: $cli_args"
echo "##########################################################################"


echo "Executing from $dir"
echo "Creating masternode account"
command="$dir/pivx-cli $cli_args getaccountaddress $masternode_name"
echo "  command: $command"
masternode_account=`$command`

echo "Generating masternode Private Key"
command="$dir/pivx-cli $cli_args masternode genkey"
echo "  command: $command"
masternode_private_key=`$command`

echo "Sending $masternode_coins_amount to $masternode_account"
command="$dir/pivx-cli $cli_args sendtoaddress $masternode_account $masternode_coins_amount"
masternode_tx=`$command`
echo "  command: $command"


echo "Generating $masternode_conf_file file"
echo "server=1" > $masternode_conf_file
echo "daemon=1" >> $masternode_conf_file
echo "rpcuser=$masternode_user"  >> $masternode_conf_file
echo "rpcpassword=$masternode_password"  >> $masternode_conf_file
echo "listen=1"  >> $masternode_conf_file
echo "staking=0"  >> $masternode_conf_file

echo "masternode=1"  >> $masternode_conf_file
echo "masternodeprivkey=$masternode_private_key"  >> $masternode_conf_file
echo "masternodeaddr=$masternode_onion_address"   >> $masternode_conf_file

echo "Generating $control_wallet file"
command="$dir/pivx-cli $cli_args gettransaction $masternode_tx"
echo "  command: $command"
hex_raw_transaction=`$command | jq .hex | tr -d \"`

echo "$hex_raw_transaction"
command="$dir/pivx-cli $cli_args decoderawtransaction $hex_raw_transaction"
echo "  command: $command"
nValue=`$command | jq .vout[] | jq select\(.value==$masternode_coins_amount\) | jq .n`

echo "$masternode_name masternode_onion_address:$masternode_port $masternode_private_key $masternode_tx $nValue" > $control_wallet

echo "Please the configurations are in the files" 
echo "This file will be used for masternode configuration: $masternode_conf_file"
echo "This file will be used controle wallet configuration: $control_wallet"
