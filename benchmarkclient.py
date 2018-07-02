from os import system


def main():
    for i in range(0, 100):
        print(str(i) + " % done")
        system("./build/csync -h localhost -p 12345 -u user0 -pass password0 -f benchmark/")

if __name__ == '__main__':
    main()
