# source makefile.variable

# folder=lib
# path=$ipath$folder

# if [ $# -lt 1 ]
# then
#     echo "usage: $0 <path-results>"
#     exit
# fi

#LD_LIBRARY_PATH=$path GSL_RNG_SEED=1992  ./cloth 
GSL_RNG_SEED=1992  ./cloth

python batch-means.py

#cp  *.csv *.json log $1
