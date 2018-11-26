# if [ $# = 0 ]
# then
#     echo "usage: $0 <project_path>"
#     exit
# fi

# ppath=$1

# if [ ! -d $ppath ]; then
#     echo "invalid project path"
#     exit
# fi

# cd $ppath
cd ..

if [ ! -d results/ ]; then
    mkdir results/
fi


npayments=(30000 42000 60000 90000 120000 180000 360000)

for i in "${npayments[@]}"
do

    if [ ! -d results/npayments_$i ]; then
        mkdir results/npayments_$i
    fi

    if [ -z "$(ls -A results/npayments_$i)" ]; then

        echo "
{
  \"PaymentMean\":0.01,
  \"NPayments\":$i,
  \"NPeers\":10000,
  \"NChannels\":5,
  \"PUncooperativeBeforeLock\":0.0001,
  \"PUncooperativeAfterLock\":0.0001,
  \"PercentageRWithholding\":0.0,
  \"Gini\":1,
  \"Sigma\":-1,
  \"PercentageSameDest\":0.0,
  \"SigmaAmount\":1,
  \"Capacity\":3e8
}
" > preinput.json

        make run > log

        cp preinput.json  peer.csv channel.csv channelInfo.csv payment.csv channel_output.csv channelInfo_output.csv payment_output.csv log results/npayments_$i

        #python2.7 scripts/json_to_csv.py $ppath/results/npayments_$i/output.json $ppath/results/npayments.csv npayments $i

    fi
done

