#!/bin/sh

masternode_coins_amount=500
coin=kore

if [ $# -lt 3 ]
then
echo "#############################################################################"
echo "#############################################################################"
echo "#############################################################################"
echo "## "
echo "## This is a Kore script to help create a masternode configuration file"
echo "## masternode_creation.sh <network> <name> <onion-address> <user> <password>"
echo "## Parameters"
echo "##   network: mainnet or testnet"
echo "##   name: a name for your masternode"
echo "##   onion-address: your masternode onion address"
echo "##   user: control wallet rpc-user (optional)"
echo "##   password: control wallet rpc-password (option)"
echo "## example:"
echo "##   masternode_creation.sh testnet MasternodeI 443xtqzgm2vxi7fy.onion  kore 12345"
echo "## "
echo "## Requirements"
echo "##   1) run this script in your control wallet"
echo "##   2) have jq installed in the systems"
echo "##   3) you should have $masternode_coins_amount COINS in this wallet"
echo "##   4) Make sure the kored is running at your masternode, you need the masternode"
echo "##       onion address and it needs to be up to lock the coins."
echo "##"
echo "## PLEASE install jq from here https://stedolan.github.io/jq/"
echo "#############################################################################"
echo "#############################################################################"

exit 1
fi

if [ $# -eq 4 ]
$echo "## missing authentication user and password"
exit 1
fi


if [ $# -eq 5 ]
control_wallet_user=$4
control_wallet_password=$5
fi

echo "##########################################################################"
echo "####################                          ############################"
echo "#################### MASTERNODE CONFIGURATION ############################"
echo "####################                          ############################"

dir=`pwd`
user_dir=$HOME
network=$1
masternode_name=$2
masternode_onion_address=$3
masternode_user=$control_wallet_user
masternode_password=$control_wallet_password
masternode_conf_file="$dir/$masternode_name.conf"
txConfirmations=6

if [ $# -eq 5 ]
cli_args="-$network -debug -rpcuser=$control_wallet_user -rpcpassword=$control_wallet_password"
else 
cli_args="-$network -debug"
fi

if [ "$network" = "testnet" ] || [ "$network" = "TESTNET" ]
then
  masternode_port=11743
  control_wallet="$user_dir/.$coin/testnet4/masternode.conf"
else
  masternode_port=10743
  control_wallet="$user_dir/.$coin/masternode.conf"
fi

echo "## "
echo "## Parameters used"
echo "##   network: $network"
echo "##   masternode-name   : $masternode_name"
echo "##   masternode-address: $masternode_onion_address"
echo "##   masternode-port   : $masternode_port"
if [ $# -eq 5 ]
echo "##   user    : $masternode_user"
echo "##   password: $masternode_password"
fi
echo "##   cli args: $cli_args"
echo "##########################################################################"


echo "Executing from $dir"
echo "Creating masternode account"
command="$dir/kore-cli $cli_args getaccountaddress $masternode_name"
echo "  command: $command"
masternode_account=`$command`

echo "Generating masternode Private Key"
command="$dir/kore-cli $cli_args masternode genkey"
echo "  command: $command"
masternode_private_key=`$command`

echo "Sending $masternode_coins_amount to $masternode_account"
command="$dir/kore-cli $cli_args sendtoaddress $masternode_account $masternode_coins_amount"
masternode_tx=`$command`
echo "  command: $command"


echo "Generating $masternode_conf_file file"
echo "server=1" > $masternode_conf_file
echo "daemon=1" >> $masternode_conf_file
if [ $# -eq 5 ]
echo "rpcuser=$masternode_user"  >> $masternode_conf_file
echo "rpcpassword=$masternode_password"  >> $masternode_conf_file
fi
echo "listen=1"  >> $masternode_conf_file
echo "staking=0"  >> $masternode_conf_file

echo "masternode=1"  >> $masternode_conf_file
echo "masternodeprivkey=$masternode_private_key"  >> $masternode_conf_file
echo "masternodeaddr=$masternode_onion_address"   >> $masternode_conf_file

echo "Generating $control_wallet file"
command="$dir/kore-cli $cli_args gettransaction $masternode_tx"
echo "  command: $command"
hex_raw_transaction=`$command | jq .hex | tr -d \"`

echo "$hex_raw_transaction"
command="$dir/kore-cli $cli_args decoderawtransaction $hex_raw_transaction"
echo "  command: $command"
nValue=`$command | jq .vout[] | jq select\(.value==$masternode_coins_amount\) | jq .n`

echo "$masternode_name $masternode_onion_address:$masternode_port $masternode_private_key $masternode_tx $nValue" >> $control_wallet

echo "##########################################################################"
echo "## Let's wait for the Confirmations"
echo "##########################################################################"
command="$dir/kore-cli $cli_args gettransaction $masternode_tx"

confirmations=`$command | jq .confirmations`
while [ $confirmations != $txConfirmations ]
do
  echo " Waiting for $txConfirmations confirmations, so far we have $confirmations"
  sleep 10
  confirmations=`$command | jq .confirmations`
done
echo ""
echo " COOL ! We got all $confirmations confirmations"
echo ""

echo "##########################################################################"
echo "## Let's Make sure the $masternode_coins_amount $coin are locked"
echo "##########################################################################"
command="$dir/kore-cli $cli_args listlockunspent"

tx_locked=`$command | jq .[].txid | scan\("$masternode_tx"\)`
if [ "$tx_locked" = "$masternode_tx" ]
then
echo "## GOOD the transaction $masternode_tx locked $masternode_coins_amount $coin"
else
echo "## BAD NEWS !!!"
echo "## Something Went Wrong, the coins are not locked !"
echo "## Please take a look at the command: $command"
exit 0
fi

echo ""
echo "##########################################################################"
echo "## Congratulations !!!"
echo "## your Masternode is ready to be started !!!             "
echo "## "
echo "## Now you need to perform the following steps"
echo "##   1. Change your masternode $coin.conf with the parameters found here:"
echo "##      $masternode_conf_file"
echo "##   2. Restart this control Wallet, so the masternode.conf will take effect."
echo "##   3. Activate your masternode with the command:"
echo "##    $dir/test/masternode/masternode_activation.sh $dir/kore-cli $cli_args $masternode_name"
echo "##########################################################################"



