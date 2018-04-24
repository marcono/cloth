if [ $# = 0 ]
then
    echo "usage: $0 <project_path>"
    exit
fi

ppath=$1

if [ ! -d $ppath ]; then
    echo "invalid project path"
    exit
fi

cd $ppath
if [ ! -d results/ ]; then
    mkdir results
fi


sigma=(0 1 10 -1)

for i in "${sigma[@]}"
do

    if [ ! -d results/sigma_$i ]; then
        mkdir results/sigma_$i
    fi

    if [ -z "$(ls -A results/sigma_$i)" ]; then

        echo "
{
  \"PaymentMean\":0.01,
  \"NPayments\":30000,
  \"NPeers\":1000000,
  \"NChannels\":5,
  \"PUncooperativeBeforeLock\":0.0001,
  \"PUncooperativeAfterLock\":0.0001,
  \"PercentageRWithholding\":0.0,
  \"Gini\":1,
  \"Sigma\":$i
}
" > preinput.json

        make run > log

        cp preinput.json output.json peer.csv channel.csv channelInfo.csv payment.csv channel_output.csv channelInfo_output.csv payment_output.csv log results/sigma_$i

    fi
done


nchan=(2 5 8 11)

for i in "${nchan[@]}"
do

    if [ ! -d results/nchannels_$i ]; then
        mkdir results/nchannels_$i
    fi

    if [ -z "$(ls -A results/nchannels_$i)" ]; then

        echo "
{
  \"PaymentMean\":0.01,
  \"NPayments\":30000,
  \"NPeers\":1000000,
  \"NChannels\":$i,
  \"PUncooperativeBeforeLock\":0.0001,
  \"PUncooperativeAfterLock\":0.0001,
  \"PercentageRWithholding\":0.0,
  \"Gini\":1,
  \"Sigma\":-1
}
" > preinput.json

        make run > log

        cp preinput.json output.json peer.csv channel.csv channelInfo.csv payment.csv channel_output.csv channelInfo_output.csv payment_output.csv log results/nchannels_$i

    fi
done

gini=(1 2 3 4 5)

for i in "${gini[@]}"
do

    if [ ! -d results/gini_$i ]; then
        mkdir results/gini_$i
    fi

    if [ -z "$(ls -A results/gini_$i)" ]; then

        echo "
{
  \"PaymentMean\":0.01,
  \"NPayments\":30000,
  \"NPeers\":1000000,
  \"NChannels\":5,
  \"PUncooperativeBeforeLock\":0.0001,
  \"PUncooperativeAfterLock\":0.0001,
  \"PercentageRWithholding\":0.0,
  \"Gini\":$i,
  \"Sigma\":-1
}
" > preinput.json

        make run > log

        cp preinput.json output.json peer.csv channel.csv channelInfo.csv payment.csv channel_output.csv channelInfo_output.csv payment_output.csv log results/gini_$i

    fi
done

