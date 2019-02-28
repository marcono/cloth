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

results=/home/marco/nexa-cloud/results

npayments=(1000 2500 5000 7500 10000 25000 50000 75000 100000 250000 500000 750000 1000000)

for i in "${npayments[@]}"
do

    payment_mean=`echo "scale=10; 1/($i/1000)" | bc | awk '{printf "%f", $0}'`
    #payment_mean=`printf %.3f\\n "$(( 1/payment_mean ))"`

    if [ ! -d $results/npayments_$i ]; then
        mkdir $results/npayments_$i
    fi

    if [ -z "$(ls -A $results/npayments_$i)" ]; then

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
  \"SigmaAmount\":10,
  \"Capacity\":3e8
}
" > preinput.json

        ./run.sh 1992 0 > log

        mv preinput.json channel_output.csv channelInfo_output.csv payment_output.csv peer_output.csv log $results/npayments_$i

        python2.7 scripts/batch_means.py $results/npayments_$i/

    fi
done

