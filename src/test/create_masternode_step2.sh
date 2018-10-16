#!/bin/sh


if [ $# -lt 5 ]
then
echo "#############################################################################"
echo "#############################################################################"
echo "#############################################################################"
echo "## "
echo "## This is a Kore script to help create a masternode configuration file"
echo "## This phase we will check if the transaction is already in the blockchain "
echo "## create_masternode_step2 <network> <account> <deposit> <user> <password>"
echo "## Parameters"
echo "##   <network>      : mainnet or testnet"
echo "##   <account>      : masternode account"
echo "##   <deposit>      : masternode money requirement"
echo "##   <rpc-user>     : control wallet rpc user"
echo "##   <rpc-passoword>: control wallet rpc password"
echo "##   The amount in the address may change, once the masternode can receive rewards"
echo "#############################################################################"
echo "#############################################################################"
exit 1
fi

network=$1
masternode_account=$2
masternode_deposit=$3
control_wallet_user=$4
control_wallet_password=$5

dir=`pwd`/..

cli_args="-$network -debug -rpcuser=$control_wallet_user -rpcpassword=$control_wallet_password"

echo "## "
echo "## Parameters used"
echo "##   network: $network"
echo "##   masternode account: $masternode_account"
echo "##   masternode deposit: $masternode_deposit"
echo "##   rpc-user    : $control_wallet_user"
echo "##   rpc-password: $control_wallet_password"
echo "##   cli args: $cli_args"
echo "##########################################################################"

command="$dir/kore-cli $cli_args listaddressgroupings"
result=$($command | jq '{address:(.[][])}' | jq select\(.address[0]==\"$masternode_account\"and.address[1]==$masternode_deposit\) )
echo $result
