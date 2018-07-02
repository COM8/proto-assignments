from os import system


def main():
    for i in range(0, 100):
        print(str(i) + " % done")
        system("./build/csync -s -p 12345 -cc 0 >> result.txt")


if __name__ == '__main__':
    main()
