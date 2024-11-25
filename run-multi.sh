cd build/bin/multi
touch ./server.log
tail -f ./server.log &
./multi 