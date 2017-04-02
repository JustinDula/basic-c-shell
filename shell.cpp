#include <vector>
#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <sstream>
#include <sys/wait.h>
#include <cstring>

#define MAXARGS 32
#define MAXARGLEN 128

using namespace std;

void parent_sighand(int sig)
{
	// does nothing for now
}

void child_sighand(int sig)
{
	exit(EXIT_FAILURE);
}

istream& getinput(string &l)
{
	//char temp[WD_MAXLEN];
	//getcwd(temp, WD_MAXLEN);
	//cout << temp << "$ ";
	
	cout << "$$ ";
	return getline(cin, l);
}

int parse(string line, char* args[MAXARGS])
{
	stringstream ss(line);
	//string t;
	char* temp;
	
	int i = 0;
	while (ss >> temp
		&& i < MAXARGS)
	{
		args[i] = temp; 
		temp = new char;
		++i;
	}
	
	/*
	for (int ii = 0; ii < i; ++ii)
		cout << args[ii] << " ";

	 cout << endl; // */

	// for debugging purposes
	//for (int j = 0; j <= i; ++j) cout << args[j] << " "; cout << endl;
	
	/*	
	for ( ; i < MAXARGS; ++i)
		args[i] = 0;//*/
	
	return 0;
}

int run (char ** args)
{
	pid_t pid;
	int status;
	
	pid = fork();
	if (pid == 0)
	{
		// child process
		if (execvp(args[0], args) == -1)
			cerr << "Error: execvp failed.\n";
		//free(args);

		signal(SIGUSR1, child_sighand); 
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

bool builtins(char * argv[MAXARGS])
{
	if (strcmp(argv[0], "exit") == 0)
	{
		exit(EXIT_SUCCESS);
		return true;
	}
	else if (strcmp(argv[0], "cd") == 0)
	{
		if (argv[1] == 0)
			cerr << "Directory required.\n";
		else
			chdir(argv[1]);
		return true;
	}
	return false;
}

void main_loop()
{

	string line;
	int status;
	
	while (getinput(line))
	{		
		char * argv[MAXARGS];
		parse(line, argv);
		//cout << argv[0] << endl << argv[1] << endl;
		if (argv[0] != 0) 
			if (! builtins(argv))
				status = run(argv);
		
	}
}

int main(int argc, char ** argv)
{
	
	cout << flush;
	cerr << flush;
	main_loop();
	
	return 0;
}


