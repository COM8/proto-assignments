package main

//only standart library
import (
	"crypto/sha256"
	"flag"
	"fmt"
	"io"
	"io/ioutil"
	"os"
)

//usage: ./main -p 4000 -h myhost -f . -s
func main() {
	var hostname, folderpath string
	hasher := sha256.New()
	port := flag.Int("p", 3005, "port to listen") //default value is 3005

	flag.StringVar(&hostname, "h", "", "host to connect")
	flag.StringVar(&folderpath, "f", "", "folder to upload")

	isServer := flag.Bool("s", false, "start server mode")
	flag.Parse()

	fmt.Println("port:", *port)
	fmt.Println("hostname:", hostname)
	fmt.Println("folderpath:", folderpath)
	fmt.Println("isServer:", *isServer)
	fmt.Println("\n-------")
	if *isServer {
		fmt.Println("running in server mode")
	}
	if hostname == "" {
		fmt.Println("Hostname can't be empty")
		os.Exit(1)
	}
	if folderpath == "" {
		fmt.Println("folderpath can't be empty")
		os.Exit(1)
	}

	files, _ := ioutil.ReadDir(folderpath)
	for _, file := range files {
		f, _ := os.Open(file.Name())
		defer f.Close()
		io.Copy(hasher, f)
		fmt.Printf("%s --> %x\n", file.Name(), hasher.Sum(nil))
	}
}
