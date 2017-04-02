#include <vector>
#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <sstream>
#include <sys/wait.h>
#include <cstring>

using namespace std;

istream& getinput(string &line);
bool builtins(vector< const char * > argv);
int parse(string line, vector< const char* > &args);
int run (vector< const char * > args);
void printargs(vector<const char *> a);
void child_sighand(int sig);
void main_loop();

int main(int argc, char ** argv)
{

	cout << flush;
	cerr << flush;
	main_loop();
	
	return 0;
}

void main_loop()
{

	string line;
	int status;
	while (getinput(line))
	{		
		vector<const char *> argv;
		parse(line, argv);
		//cout << argv[0] << endl << argv[1] << endl;
		printargs(argv);
		if (argv[0] != 0) 
			if (! builtins(argv))
				status = run(argv);
		
	}
}

int parse(string line, vector<const char* > &args)
{
	stringstream ss(line);
	string t;
	
	args.clear();
	
	int i = 0;
	while (ss >> t)
	{
		char * push = new char[t.size() + 1];
		strcpy(push, t.c_str());
		args.push_back(push);
		++i;
	}
	
	return 0;
}

bool builtins(vector< const char * > argv)
{
	if (strcmp(argv[0], "exit") == 0)
	{
		exit(EXIT_SUCCESS);
		return true;
	}
	else if (strcmp(argv[0], "cd") == 0)
	{
		if (argv.size() < 2)
			cerr << "Directory required.\n";
		else
			chdir(argv[1]);
		return true;
	}
	return false;
}

int run (vector< const char * > args)
{
	pid_t pid;
	int status;
	
	pid = fork();
	if (pid == 0)
	{
		// child process
		signal(SIGUSR1, child_sighand); 
		if (execvp(args[0], (char**) args.data()) == -1)
			cerr << "Error: execvp failed.\n";
		//free(args);
		exit(EXIT_FAILURE);
	} 
	else if (pid < 0)
	{
		// fork failed
		cerr << "Error: Fork failed.\n";
	}
	else
	{
		// parent process
		waitpid(pid, &status, WUNTRACED);
	}
	
	return 1;
}



istream& getinput(string &l)
{
	//char temp[WD_MAXLEN];
	//getcwd(temp, WD_MAXLEN);
	//cout << temp << "$ ";
	
	cout << "$$ ";
	return getline(cin, l);
}

void printargs(vector<const char *> argv)
{
	cout << "ARGS: ";
	for (int i = 0; i < argv.size() ; ++i)
		cout << argv[i] << " ";
	cout << endl;
}

void parent_sighand(int sig)
{
	// does nothing for now
}

void child_sighand(int sig)
{
	exit(EXIT_FAILURE);
}


