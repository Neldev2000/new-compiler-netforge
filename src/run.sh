make clean
make all
cd ../bin
./mikrotik_compiler ../examples/new.dsl  ../generated/test.rsc
cd ../src
cat ../generated/test.rsc