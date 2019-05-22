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

# if [ ! -d results/ ]; then
#     mkdir results/
# fi

results=/home/marco/nexa-cloud/results/best-case


sigma_amounts=(10.0)
npayments=(10000 20000 30000 40000 50000 60000 70000 80000 90000 100000 200000 300000 400000 500000 600000 700000 800000 900000 1000000)


for i in "${npayments[@]}"
do

    payment_mean=`echo "scale=10; 1/($i/1000)" | bc | awk '{printf "%f", $0}'`
    #payment_mean=`printf %.3f\\n "$(( 1/payment_mean ))"`

    if [ ! -d $results/npayments_$i ]; then
        mkdir $results/npayments_$i
    fi

    for j in "${sigma_amounts[@]}"
    do
        if [ ! -d $results/npayments_$i/sigma_$j ]; then
            mkdir $results/npayments_$i/sigma_$j
        fi

        if [ -z "$(ls -A $results/npayments_$i/sigma_$j)" ]; then

        echo "
{
  \"PaymentMean\":$payment_mean,
  \"NPayments\":$i,
  \"NPeers\":10000,
  \"NChannels\":5,
  \"PUncooperativeBeforeLock\":0.0,
  \"PUncooperativeAfterLock\":0.0,
  \"PercentageRWithholding\":0.0,
  \"Gini\":1,
  \"Sigma\":-1,
  \"PercentageSameDest\":0.0,
  \"SigmaAmount\":$j,
  \"Capacity\":3e8
}
" > preinput.json

        ./run.sh $results/npayments_$i/sigma_$j > log

        fi
    done

done

# sigma_amounts=(1 1.1 1.2 1.3 1.4 1.5 1.6 1.7 1.8 1.9 2.0)
# npay=(1000 2500 5000 10000 25000 50000 100000)

# for i in "${sigma_amounts[@]}"
# do

#     if [ ! -d $results/amounts_$i ]; then
#         mkdir $results/amounts_$i
#     fi

#     if [ -z "$(ls -A $results/amounts_$i)" ]; then

#         echo "
# {
#   \"PaymentMean\":1,
#   \"NPayments\":1000,
#   \"NPeers\":10000,
#   \"NChannels\":5,
#   \"PUncooperativeBeforeLock\":0.0,
#   \"PUncooperativeAfterLock\":0.0,
#   \"PercentageRWithholding\":0.0,
#   \"Gini\":1,
#   \"Sigma\":-1,
#   \"PercentageSameDest\":0.0,
#   \"SigmaAmount\":$i,
#   \"Capacity\":3e8
# }
# " > preinput.json

#         ./run.sh 1992 0 > log

#         mv preinput.json channel_output.csv channelInfo_output.csv payment_output.csv peer_output.csv log $results/amounts_$i

#         python2.7 scripts/batch_means.py $results/amounts_$i/

#     fi
# done

