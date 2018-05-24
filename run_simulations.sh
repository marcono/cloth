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



# nchannels=(2 8 11)

# for i in "${nchannels[@]}"
# do

#     if [ ! -d results/nchannels_$i ]; then
#         mkdir results/nchannels_$i
#     fi

#     if [ -z "$(ls -A results/nchannels_$i)" ]; then

#         echo "
# {
#   \"PaymentMean\":0.001,
#   \"NPayments\":30000,
#   \"NPeers\":100000,
#   \"NChannels\":$i,
#   \"PUncooperativeBeforeLock\":0.0001,
#   \"PUncooperativeAfterLock\":0.0001,
#   \"PercentageRWithholding\":0.0,
#   \"Gini\":1,
#   \"Sigma\":-1,
#   \"PercentageSameDest\":0.0
# }
# " > preinput.json

#         make run > log

#         cp preinput.json output.json peer.csv channel.csv channelInfo.csv payment.csv channel_output.csv channelInfo_output.csv payment_output.csv log results/nchannels_$i

#         python2.7 scripts/json_to_csv.py $ppath/results/nchannels_$i/output.json $ppath/results/nchannels.csv nchannels $i

#     fi
# done


sigma=(0 1 10 -1)

for i in "${sigma[@]}"
do

    if [ ! -d results/sigma_$i ]; then
        mkdir results/sigma_$i
    fi

    if [ -z "$(ls -A results/sigma_$i)" ]; then

        echo "
{
  \"PaymentMean\":0.001,
  \"NPayments\":30000,
  \"NPeers\":100000,
  \"NChannels\":5,
  \"PUncooperativeBeforeLock\":0.0001,
  \"PUncooperativeAfterLock\":0.0001,
  \"PercentageRWithholding\":0.0,
  \"Gini\":1,
  \"Sigma\":$i,
  \"PercentageSameDest\":0.0
}
" > preinput.json

        make run > log

        cp preinput.json output.json peer.csv channel.csv channelInfo.csv payment.csv channel_output.csv channelInfo_output.csv payment_output.csv log results/sigma_$i

        python2.7 scripts/json_to_csv.py $ppath/results/sigma_$i/output.json $ppath/results/sigma.csv sigma $i

    fi
done


samedest=(0.1 0.2 0.4)

for i in "${samedest[@]}"
do

    if [ ! -d results/samedest_$i ]; then
        mkdir results/samedest_$i
    fi

    if [ -z "$(ls -A results/samedest_$i)" ]; then

        echo "
{
  \"PaymentMean\":0.001,
  \"NPayments\":30000,
  \"NPeers\":100000,
  \"NChannels\":5,
  \"PUncooperativeBeforeLock\":0.0001,
  \"PUncooperativeAfterLock\":0.0001,
  \"PercentageRWithholding\":0.0,
  \"Gini\":1,
  \"Sigma\":-1,
  \"PercentageSameDest\":$i
}
" > preinput.json

        make run > log

        cp preinput.json output.json peer.csv channel.csv channelInfo.csv payment.csv channel_output.csv channelInfo_output.csv payment_output.csv log results/samedest_$i

        python2.7 scripts/json_to_csv.py $ppath/results/samedest_$i/output.json $ppath/results/samedest.csv samedest $i

    fi
done



npeers=(200000 500000 700000 1000000)

for i in "${npeers[@]}"
do

    if [ ! -d results/npeers_$i ]; then
        mkdir results/npeers_$i
    fi

    if [ -z "$(ls -A results/npeers_$i)" ]; then

        echo "
{
  \"PaymentMean\":0.001,
  \"NPayments\":30000,
  \"NPeers\":$i,
  \"NChannels\":5,
  \"PUncooperativeBeforeLock\":0.0001,
  \"PUncooperativeAfterLock\":0.0001,
  \"PercentageRWithholding\":0.0,
  \"Gini\":1,
  \"Sigma\":-1,
  \"PercentageSameDest\":0.0
}
" > preinput.json

        make run > log

        cp preinput.json output.json peer.csv channel.csv channelInfo.csv payment.csv channel_output.csv channelInfo_output.csv payment_output.csv log results/npeers_$i

        python2.7 scripts/json_to_csv.py $ppath/results/npeers_$i/output.json $ppath/results/npeers.csv npeers $i

    fi
done

gini=(2 3 4 5)

for i in "${gini[@]}"
do

    if [ ! -d results/gini_$i ]; then
        mkdir results/gini_$i
    fi

    if [ -z "$(ls -A results/gini_$i)" ]; then

        echo "
{
  \"PaymentMean\":0.001,
  \"NPayments\":30000,
  \"NPeers\":100000,
  \"NChannels\":5,
  \"PUncooperativeBeforeLock\":0.0001,
  \"PUncooperativeAfterLock\":0.0001,
  \"PercentageRWithholding\":0.0,
  \"Gini\":$i,
  \"Sigma\":-1,
  \"PercentageSameDest\":0.0
}
" > preinput.json

        make run > log

        cp preinput.json output.json peer.csv channel.csv channelInfo.csv payment.csv channel_output.csv channelInfo_output.csv payment_output.csv log results/gini_$i

        python2.7 scripts/json_to_csv.py $ppath/results/gini_$i/output.json $ppath/results/gini.csv gini $i

    fi
done

npayments=(120000 210000 300000)

for i in "${npayments[@]}"
do

    if [ ! -d results/npayments_$i ]; then
        mkdir results/npayments_$i
    fi

    if [ -z "$(ls -A results/npayments_$i)" ]; then

        echo "
{
  \"PaymentMean\":0.001,
  \"NPayments\":$i,
  \"NPeers\":100000,
  \"NChannels\":5,
  \"PUncooperativeBeforeLock\":0.0001,
  \"PUncooperativeAfterLock\":0.0001,
  \"PercentageRWithholding\":0.0,
  \"Gini\":1,
  \"Sigma\":-1,
  \"PercentageSameDest\":0.0
}
" > preinput.json

        make run > log

        cp preinput.json output.json peer.csv channel.csv channelInfo.csv payment.csv channel_output.csv channelInfo_output.csv payment_output.csv log results/npayments_$i

        python2.7 scripts/json_to_csv.py $ppath/results/npayments_$i/output.json $ppath/results/npayments.csv npayments $i

    fi
done


samedest=(0.5)

for i in "${samedest[@]}"
do

    if [ ! -d results/samedest_$i ]; then
        mkdir results/samedest_$i
    fi

    if [ -z "$(ls -A results/samedest_$i)" ]; then

        echo "
{
  \"PaymentMean\":0.001,
  \"NPayments\":30000,
  \"NPeers\":100000,
  \"NChannels\":5,
  \"PUncooperativeBeforeLock\":0.0001,
  \"PUncooperativeAfterLock\":0.0001,
  \"PercentageRWithholding\":0.0,
  \"Gini\":1,
  \"Sigma\":-1,
  \"PercentageSameDest\":$i
}
" > preinput.json

        make run > log

        cp preinput.json output.json peer.csv channel.csv channelInfo.csv payment.csv channel_output.csv channelInfo_output.csv payment_output.csv log results/samedest_$i

        python2.7 scripts/json_to_csv.py $ppath/results/samedest_$i/output.json $ppath/results/samedest.csv samedest $i

    fi
done

