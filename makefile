G++_COMPILER=g++ # In mac I need to change this to g++-7 , so I made it a variable.
BUILD_DIR=build

client:
	make default
	./$(BUILD_DIR)/csync -h localhost -p 1234 -f

server:
	make default
	./$(BUILD_DIR)/csync -s -p 1234

default:
	make clean
	make compile

compile:
	# Create the build directory if it does not allready exist:
	mkdir -p $(BUILD_DIR)

	${G++_COMPILER} src/*.cpp src/net/*.cpp -I src/ -I src/net/ -o $(BUILD_DIR)/csync -lstdc++fs -std=c++17 -pthread

clean:
	# Only remove the build folder if it exists:
	if [ -d $(BUILD_DIR) ]; then rm -rf $(BUILD_DIR); fi

test:
	make compile
	./$(BUILD_DIR)/csync -f .vscode/ -h myhost -p 4500