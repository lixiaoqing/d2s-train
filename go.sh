set -x
make
mv extract-rules unit-test/ 
cd unit-test
./extract-rules toy.ch.dep toy.en toy.align toy.lex.s2t toy.lex.t2s
cd -
