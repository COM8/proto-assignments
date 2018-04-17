compile:
	g++ csync.cpp -o csync

clean:
	-rm a.out
	-rm csync

test:
	g++ csync.cpp -o csync -std=c++17
	./csync -f .vscode/ -h myhost -p 4500  