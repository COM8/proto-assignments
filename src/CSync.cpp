#include "CSync.h"

using namespace std;
using namespace test;

struct arg *parseParameter(int argc, char *argv[])
{
	struct arg *temp = new arg();
	temp->type = client; //assume it is client at first
	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-h") == 0)
		{

			if (argc > i + 1)
			{
				temp->host = string(argv[i + 1]);
			}
		}
		else if (strcmp(argv[i], "-p") == 0)
		{
			if (argc > i + 1)
			{
				temp->port = atoi(argv[i + 1]);
			}
		}
		else if (strcmp(argv[i], "-f") == 0)
		{
			if (argc > i + 1)
			{
				temp->dir = string(argv[i + 1]);
			}
		}
		else if (strcmp(argv[i], "-s") == 0)
		{
			temp->type = server;
		}
		else if (strcmp(argv[i], "-u") == 0)
		{
			if (argc > i + 1)
			{
				temp->user = string(argv[i + 1]);
			}
		}
		else if (strcmp(argv[i], "-pass") == 0)
		{
			if (argc > i + 1)
			{
				temp->password = string(argv[i + 1]);
			}
		}
		else if (strcmp(argv[i], "-cc") == 0)
		{
			if (argc > i + 1)
			{
				temp->maxPPS = atoi(argv[i + 1]);
			}
		}
		else if (strcmp(argv[i], "-t") == 0)
		{
			temp->runTests = true;
		}
	}
	if (temp->port == DEFAULT_PORT)
	{
		cout << "Using default port: " << temp->port << endl;
	}
	if (temp->maxPPS == DEFAULT_MAX_PPS)
	{
		cout << "Using default max packages per second: " << temp->maxPPS << endl;
	}
	if (temp->type == client)
	{
		if ((temp->host) == "")
		{
			cerr << "No hostname or ip-addr given " << endl
				 << helpString;
			exit(-1);
		}
		else if ((temp->user) == "")
		{
			cerr << "No user name given " << endl
				 << helpString;
			exit(-1);
		}
		else if ((temp->user) == "")
		{
			cerr << "No password given " << endl
				 << helpString;
			exit(-1);
		}
		else if ((temp->dir) == "")
		{
			cerr << "No sync folder given using current folder" << endl;
			temp->dir = "./";
		}
		if (!Filesystem::exists(temp->dir))
		{
			cerr << "given path doesn't exist terminating" << endl;
			exit(-1);
		}
	}
	else if (!temp->type == server)
	{
		cerr << "No mode given please specify it next time" << endl
			 << helpString;
		exit(-1);
	}

	return temp;
}

void launchServer(unsigned int port, unsigned int maxPPS)
{
	FileServer2 fS = FileServer2((unsigned short)port, maxPPS);
	fS.start();

	cout << "Server console type \"stop\" to stop the server." << endl;
	string s;
	while (true)
	{
		cout << "SERVER: ";
		cin >> s;

		if (!s.compare("stop"))
		{
			cout << "Stopping server..." << endl;
			fS.stop();
			cout << "Stopped server!" << endl;
			break;
		}
		else
		{
			cout << "Unknown command: \"" << s << "\"" << endl;
		}
	}
}

void printClientHelp()
{
	cout << endl;
	cout << "help - print this" << endl;
	cout << "stop - stop the client and exit" << endl;
	cout << "ping - ping the connected server" << endl;
	cout << "state - print the current client state" << endl;
	cout << "todo - the current transfer to do list" << endl;
	cout << endl;
}

void launchClient(unsigned int port, string host, string dir, string userName, string password)
{
	FilesystemClient *fS = new FilesystemClient(dir);
	FileClient2 *fC = new FileClient2(host, (unsigned short)port, userName, password, fS);
	fC->connect();

	cout << "Client console type \"stop\" to stop the server or \"help\" for a list of all commands." << endl;
	string s;
	while (true)
	{
		cout << "CLIENT: ";
		cin >> s;

		if (!s.compare("stop"))
		{
			cout << "Stopping client..." << endl;
			fC->disconnect();
			fS->close();
			cout << "Stopped client!" << endl;
			break;
		}
		else if (!s.compare("help"))
		{
			printClientHelp();
		}
		else if (!s.compare("ping"))
		{
			// fC->pingServer();
		}
		else if (!s.compare("todo"))
		{
			fC->printToDo();
		}
		else if (!s.compare("state"))
		{
			cout << "State: " << fC->getState() << endl;
		}
		else
		{
			cout << "Unknown command: \"" << s << "\"" << endl;
		}
	}
}

int main(int argc, char *argv[])
{
	struct arg *t = parseParameter(argc, argv);

	if (t->runTests)
	{
		UnitTests tests = UnitTests();
		tests.runTests();
	}
	else if (t->type == server)
	{
		cout << "Running in Server mode" << endl
			 << "using port: " << t->port << endl;
		launchServer(t->port, t->maxPPS);
	}
	else
	{
		cout << "Running in Client mode" << endl
			 << "connecting to: " << t->host << endl
			 << "using port: " << t->port << endl
			 << "folder path: " << t->dir << endl;
		launchClient(t->port, t->host, t->dir, t->user, t->password);
	}

	return 0;
}