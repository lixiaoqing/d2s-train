set -x
make
mv a unit-test/ 
cd unit-test
./a train.en.tree train.ch train.align.li lex.e2f lex.f2e > log
cat log
cd -
