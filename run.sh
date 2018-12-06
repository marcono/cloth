source makefile.variable

folder=lib
path=$ipath$folder

if [ $# -lt 2 ]
then
    echo "usage: $0 <seed> <is-preprocess>"
    exit
fi

LD_LIBRARY_PATH=$path GSL_RNG_SEED=$1  ./cloth $2
