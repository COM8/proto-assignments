compile:
	g++ src/csync.cpp -o build/csync

clean:
	-rm build/*

test:
	g++ src/csync.cpp -o build/csync -std=c++17
	./csync -f .vscode/ -h myhost -p 4500  