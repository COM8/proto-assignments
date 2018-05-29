G++_COMPILER=g++ # In mac I need to change this to g++-7 , so I made it a variable.
BUILD_DIR=build
DEBUG_DIR=debug
SYNC_DIR=sync

default:
	make clean
	make debug

init:
	git submodule init
	git submodule update

runClient:
	./$(DEBUG_DIR)/csync -h localhost -p 1234 -f $(DEBUG_DIR)

runServer:
	./$(DEBUG_DIR)/csync -s -p 1234

runDebugServer:
	gdb --args ./$(DEBUG_DIR)/csync "-s" "-p" "1234"

runDebugClient:
	gdb --args ./$(DEBUG_DIR)/csync "-h" "localhost" "-p" "1234" "-f" "$(DEBUG_DIR)"

runMassifClient:
	valgrind --tool=massif$(DEBUG_DIR)/csync "-s" "-p" "1234"

runMassifServer:
	valgrind --tool=massif $(DEBUG_DIR)/csync "-h" "localhost" "-p" "1234" "-f" "$(DEBUG_DIR)"

MassifClient:
	make default
	make runMassifClient

MassifServer:
	make default
	make runMassifServer

debugServer:
	make default
	make runDebugServer

debugClient:
	make default
	make runDebugClient	

client:
	make default
	make runClient

server:
	make default
	make runServer

debug:
	mkdir -p $(DEBUG_DIR)
	${G++_COMPILER} -g src/*.cpp src/net/*.cpp src/lib/hash-library/md5.cpp -I src/ -I src/net/ -I src/lib/hash-library/ -o $(DEBUG_DIR)/csync -lstdc++fs -std=c++17 -pthread

release:
	make compile

compile:
	# Create the build directory if it does not allready exist:
	mkdir -p $(BUILD_DIR)
	${G++_COMPILER} src/*.cpp src/net/*.cpp src/lib/hash-library/md5.cpp -I src/ -I src/net/ -I src/lib/hash-library/ -o $(BUILD_DIR)/csync -lstdc++fs -std=c++17 -pthread

clean:
	# Only remove the build folder if it exists:
	if [ -d $(BUILD_DIR) ]; then rm -rf $(BUILD_DIR); fi
	if [ -d $(DEBUG_DIR) ]; then rm -rf $(DEBUG_DIR); fi
	if [ -d $(SYNC_DIR) ]; then rm -rf $(SYNC_DIR); fi

test:
	make compile
	./$(DEBUG_DIR)/csync -f .vscode/ -h myhost -p 4500