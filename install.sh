if [ $# = 0 ]
then
    echo "usage: $0 <absolute-library-installation-path>"
    exit
fi

echo "ipath=$1" > makefile.variable

cd $1

git clone https://github.com/json-c/json-c.git
cd json-c
sh autogen.sh
./configure --prefix=$1
make
make install

cd ..

curl -O http://mirror2.mirror.garr.it/mirrors/gnuftp/gsl/gsl-2.4.tar.gz
tar xzf gsl-2.4.tar.gz
cd gsl-2.4
./configure --prefix=$1
make
make install

