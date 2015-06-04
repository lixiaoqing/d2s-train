set -x
make
mv a unit-test/ 
cd unit-test
./a
cd -
