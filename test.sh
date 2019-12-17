#!/bin/bash -e
echo "Compiling"
gcc vm.c -o vm
echo "Running vm"
./vm -n256 -tfifo BACKING_STORE.bin addresses.txt > out.txt
echo "Comparing with correct.txt"
diff out.txt correct.txt