if [ $# -lt 2 ]
then
    echo "usage: $0 <seed> <output-directory>"
    exit
fi

GSL_RNG_SEED=$1  ./cloth $2

python batch-means.py $2
