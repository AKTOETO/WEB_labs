cd build/bin/single
touch ./server.log
tail -f ./server.log &
./single