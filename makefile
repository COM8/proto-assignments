client:
	make default
	./build/csync -h localhost -p 1234 -f
server:
	make default
	./build/csync -s -p 1234
default:
	make clean
	make compile
compile:
	g++ src/csync.cpp -Isrc/ -o build/csync -lstdc++fs -std=c++17

clean:
	-rm build/*

test:
	g++ src/csync.cpp -Isrc/ -o build/csync -std=c++17 -lstdc++fs
	./csync -f .vscode/ -h myhost -p 4500  