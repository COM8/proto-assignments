
G++_COMPILER=g++ #In mac I need to change this to g++-7 , so I made it a variable.

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
	${G++_COMPILER} src/csync.cpp -I src/ -o build/csync -lstdc++fs -std=c++17

clean:
	-rm -r build/*

test:
	${G++_COMPILER} src/csync.cpp -I src/ -o build/csync -std=c++17 -lstdc++fs
	./build/csync -f .vscode/ -h myhost -p 4500