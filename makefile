G++_COMPILER=g++ # In mac I need to change this to g++-7 , so I made it a variable.
BUILD_DIR=build
DEBUG_DIR=debug
SYNC_DIR=sync
PORT=12345

ARGS_CLIENT=-h localhost -p $(PORT) -f $(DEBUG_DIR) -u user0 -pass password0
ARGS_CLIENT_DEBUG="-h" "localhost" "-p" "$(PORT)" "-f" "$(DEBUG_DIR)" "-u" "user0" "-pass" "password0"
ARGS_SERVER=-s -p $(PORT) -cc 0
ARGS_SERVER_DEBUG="-s" "-p" "$(PORT)" "-cc" "0"

default:
	make rebuild

rebuild:
	make clean
	make debug

init:
	git submodule init
	git submodule update

runClient:
	./$(DEBUG_DIR)/csync $(ARGS_CLIENT)

runServer:
	./$(DEBUG_DIR)/csync $(ARGS_SERVER)

runDebugServer:
	gdb --args ./$(DEBUG_DIR)/csync $(ARGS_SERVER_DEBUG)

runDebugClient:
	gdb --args ./$(DEBUG_DIR)/csync $(ARGS_CLIENT_DEBUG)

runMassifServer:
	valgrind --tool=massif $(DEBUG_DIR)/csync $(ARGS_SERVER_DEBUG)

runMassifClient:
	valgrind --tool=massif $(DEBUG_DIR)/csync $(ARGS_CLIENT_DEBUG)

runTests:
	./$(DEBUG_DIR)/csync $(ARGS_CLIENT) -t

runDebugTests:
	gdb --args ./$(DEBUG_DIR)/csync $(ARGS_SERVER_DEBUG) "-t"

massifClient:
	make default
	make runMassifClient

massifServer:
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

tests:
	make default
	make runTests

debugTests:
	make default
	make runDebugTests

debug:
	mkdir -p $(DEBUG_DIR)
	${G++_COMPILER} -g src/*.cpp src/sec/*.cpp src/net/*.cpp src/test/*.cpp src/lib/hash-library/md5.cpp src/lib/hash-library/crc32.cpp -I src/ -I src/sec/ -I src/test/ -I src/net/ -I src/lib/hash-library/ -o $(DEBUG_DIR)/csync -lstdc++fs -std=c++17 -pthread

release:
	make compile

compile:
	# Create the build directory if it does not allready exist:
	mkdir -p $(BUILD_DIR)
	${G++_COMPILER} src/*.cpp src/net/*.cpp src/sec/*.cpp src/test/*.cpp src/lib/hash-library/md5.cpp src/lib/hash-library/crc32.cpp -I src/ -I src/sec/ -I src/net/ -I src/test/ -I src/lib/hash-library/ -o $(BUILD_DIR)/csync -lstdc++fs -std=c++17 -pthread


clean:
	# Only remove the build folder if it exists:
	if [ -d $(BUILD_DIR) ]; then rm -rf $(BUILD_DIR); fi
	if [ -d $(DEBUG_DIR) ]; then rm -rf $(DEBUG_DIR); fi
	if [ -d $(SYNC_DIR) ]; then rm -rf $(SYNC_DIR); fi
