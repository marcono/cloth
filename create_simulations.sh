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

if [ ! -d simulations/ ]; then
    mkdir simulations
fi



nchannels=(2 8 11)

for i in "${nchannels[@]}"
do

    if [ ! -d simulations/nchannels_$i ]; then

        cp -r PaymentNetworkSim/. simulations/nchannels_$i

        echo "
{
  \"PaymentMean\":0.001,
  \"NPayments\":30000,
  \"NPeers\":100000,
  \"NChannels\":$i,
  \"PUncooperativeBeforeLock\":0.0001,
  \"PUncooperativeAfterLock\":0.0001,
  \"PercentageRWithholding\":0.0,
  \"Gini\":1,
  \"Sigma\":-1,
  \"PercentageSameDest\":0.0
}
" > simulations/nchannels_$i/preinput.json

    fi
done


sigma=(0 1 10 -1)

for i in "${sigma[@]}"
do

    if [ ! -d simulations/sigma_$i ]; then

        cp -r PaymentNetworkSim/. simulations/sigma_$i

        echo "
{
  \"PaymentMean\":0.001,
  \"NPayments\":30000,
  \"NPeers\":100000,
  \"Sigma\":$i,
  \"PUncooperativeBeforeLock\":0.0001,
  \"PUncooperativeAfterLock\":0.0001,
  \"PercentageRWithholding\":0.0,
  \"Gini\":1,
  \"Sigma\":-1,
  \"PercentageSameDest\":0.0
}
" > simulations/sigma_$i/preinput.json

    fi

done


samedest=(0.1 0.2 0.4 0.5)

for i in "${samedest[@]}"
do

    if [ ! -d simulations/samedest_$i ]; then

        cp -r PaymentNetworkSim/. simulations/samedest_$i

        echo "
{
  \"PaymentMean\":0.001,
  \"NPayments\":30000,
  \"NPeers\":100000,
  \"Samedest\":$i,
  \"PUncooperativeBeforeLock\":0.0001,
  \"PUncooperativeAfterLock\":0.0001,
  \"PercentageRWithholding\":0.0,
  \"Gini\":1,
  \"Sigma\":-1,
  \"PercentageSameDest\":0.0
}
" > simulations/samedest_$i/preinput.json

    fi

done



npeers=(200000 500000 700000 1000000)

for i in "${npeers[@]}"
do


    if [ ! -d simulations/npeers_$i ]; then

        cp -r PaymentNetworkSim/. simulations/npeers_$i

        echo "
{
  \"PaymentMean\":0.001,
  \"NPayments\":30000,
  \"NPeers\":100000,
  \"Npeers\":$i,
  \"PUncooperativeBeforeLock\":0.0001,
  \"PUncooperativeAfterLock\":0.0001,
  \"PercentageRWithholding\":0.0,
  \"Gini\":1,
  \"Sigma\":-1,
  \"PercentageSameDest\":0.0
}
" > simulations/npeers_$i/preinput.json

    fi


done

gini=(2 3 4 5)

for i in "${gini[@]}"
do


    if [ ! -d simulations/gini_$i ]; then

        cp -r PaymentNetworkSim/. simulations/gini_$i

        echo "
{
  \"PaymentMean\":0.001,
  \"NPayments\":30000,
  \"NPeers\":100000,
  \"Gini\":$i,
  \"PUncooperativeBeforeLock\":0.0001,
  \"PUncooperativeAfterLock\":0.0001,
  \"PercentageRWithholding\":0.0,
  \"Gini\":1,
  \"Sigma\":-1,
  \"PercentageSameDest\":0.0
}
" > simulations/gini_$i/preinput.json

    fi


done

npayments=(120000 210000 300000)

for i in "${npayments[@]}"
do


    if [ ! -d simulations/npayments_$i ]; then

        cp -r PaymentNetworkSim/. simulations/npayments_$i

        echo "
{
  \"PaymentMean\":0.001,
  \"NPayments\":30000,
  \"NPeers\":100000,
  \"Npayments\":$i,
  \"PUncooperativeBeforeLock\":0.0001,
  \"PUncooperativeAfterLock\":0.0001,
  \"PercentageRWithholding\":0.0,
  \"Gini\":1,
  \"Sigma\":-1,
  \"PercentageSameDest\":0.0
}
" > simulations/npayments_$i/preinput.json

    fi


done

