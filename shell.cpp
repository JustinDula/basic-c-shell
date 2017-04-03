#include <vector>
#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <fcntl.h>
#include <string>
#include <sstream>
#include <sys/wait.h>
#include <cstring>

#define REDIRECT_NO -1
#define REDIRECT_IN 0
#define REDIRECT_OUT 1
#define REDIRECT_APP 2

#define STD_IN 0
#define STD_OUT 1
#define STD_ERR 2

using namespace std;


// a clown fiesta of declarations
istream& getinput(string &line);
bool builtins(vector< const char * > argv);
vector<const char *> parse_to_vector(string line);
int start_full_command (vector< const char * > args);
void printargs(vector<const char *> a);
void child_sighand(int sig);
void parent_sighand(int sig);
vector < vector<const char *> > split_on_pipes(vector<const char *> args);
int spawn_single_command(int i_ID, int o_ID, vector<const char *> argv);
int fork_commands(vector < vector < const char * > > commands, int redirect, const char* redirectLocation);
int execvec(vector<const char *> argv);

int main(int argc, char ** argv)
{

	// flush the outputs
	cout << flush;
	cerr << flush;
	
	string line;
	int status;
	while (getinput(line))
	{		
		vector<const char *> argv = parse_to_vector(line);
		//printargs(argv);
		if (argv[0] != 0) 
			if (! builtins(argv))
				status = start_full_command(argv);
		
	}
	
	return 0;
}

// parses the line and fills the given vector with its contents
vector<const char *> parse_to_vector(string line)
{
	stringstream ss(line);
	string t;
	
	vector<const char *> args;
	
	args.clear();
	
	int i = 0;
	while (ss >> t)
	{
		char * push = new char[t.size() + 1];
		strcpy(push, t.c_str());
		args.push_back(push);
		++i;
	}
	
	return args;
}

// checks if it's a builtin and does it 
// does exit and cd 
bool builtins(vector< const char * > argv)
{
	// handle "exit"
	if (strcmp(argv[0], "exit") == 0)
	{
		exit(EXIT_SUCCESS);
		return true;
	}
	// handle "cd"
	else if (strcmp(argv[0], "cd") == 0)
	{
		if (argv.size() != 2)
			cerr << "CD must have exactly one argument";
		else
			chdir(argv[1]);
		return true;
	}
	return false;
}

// execvp's the given vector
int execvec(vector<const char *> argv)
{
	// please don't ask why this is necessary
	// all i know is that if failed if there 
	// were an even, nonzero number of arguments past the command
	if (argv.size() > 2 || argv.size() % 2 == 1)
		argv.push_back((const char*)0);
	
	// does the actual execution
	if (execvp(argv[0], (char**) argv.data()) == -1)
	{
			cerr << "Error: execvp failed.\n";
			exit(EXIT_FAILURE);
			return -1;
	}
	return 0;
}

// called by fork commands to do the single command
int spawn_single_command(int i_ID, int o_ID, vector<const char *> argv)
{
	pid_t pid = fork();
	// updates the inputs and outputs to their appropriate values
	if (pid == 0)
	{
		if (i_ID != 0)
		{
			dup2(i_ID, 0);
			close(i_ID);
		}
		if (o_ID != 1)
		{
			dup2(o_ID, 1);
			close(o_ID);
		}
		return execvec(argv);
	}
	
	return pid;
}

// forks off the individual commands as necessary
int fork_commands(vector < vector < const char * > > commands, int redirect, const char* redirectLocation)
{
	pid_t pid;
	int fd[2];
	
	// if there's input redirection
	if (redirect == REDIRECT_IN)
	{
		int i_ID = open(redirectLocation, O_RDONLY);
		dup2(i_ID, STD_IN);
		close(i_ID);
	}
	
	
	int i_ID;
	int i = 0;
	// for all but the last command
	for ( ; i < commands.size() - 1; ++i)
	{
		pipe(fd);
		//cout << fd[0] << "-" << fd[1] << endl;
		spawn_single_command(i_ID, fd[1], commands[i]);
		close(fd[1]);
		i_ID = fd[0];
	}
	
	if (i_ID) // if the output of the previous is not standard input
	{
		dup2 (i_ID, STD_IN);
	}
	
	// if there is file redirection
	if (redirect == REDIRECT_APP || redirect == REDIRECT_OUT)
	{
		int o_ID;
		if (redirect == REDIRECT_OUT)
			o_ID = open(redirectLocation, O_WRONLY | O_TRUNC | O_CREAT, 0666);
		else
			o_ID = open(redirectLocation, O_WRONLY | O_APPEND | O_CREAT, 0666);
		dup2(o_ID, STD_OUT);
		close(o_ID);
	}
	
	return execvec(commands[i]);
}

// starts the full command execution
int start_full_command (vector< const char * > args)
{
	pid_t pid;
	int status;
	
	signal(SIGUSR1, parent_sighand);

	pid = fork();
	if (pid == 0)
	{
		// child process
		int redirect = REDIRECT_NO;
		const char * redirectLocation = (const char*)0;
		
		if (args.size() >= 3)
		{
			int i = args.size() - 2;
			
			if (strcmp(args[i], "<") == 0)
				redirect = REDIRECT_IN;
			else if (strcmp(args[i], ">>") == 0)
				redirect = REDIRECT_APP;
			else if (strcmp(args[i], ">") == 0)
				redirect = REDIRECT_OUT;
			
			//cout << "RED" << redirect << endl;
			
			if (redirect != REDIRECT_NO)
			{
				redirectLocation = args.back();
				
				args.pop_back();
				args.pop_back();
			}
		}
		
		// split to commands
		vector < vector < const char * > > commands = split_on_pipes(args);
		
		//for (unsigned int i = 0; i < commands.size(); ++i) printargs(commands[i]);
	
		
		signal(SIGUSR1, child_sighand); 
		fork_commands(commands, redirect, redirectLocation);
		exit(EXIT_FAILURE);
	} 
	else if (pid < 0)
	{
		// fork failed
		cerr << "Error: Fork failed.\n";
	}
	
	// wait for the kiddo to finish
	if ((pid =waitpid(pid, &status, 0)) < 0)
		cerr << "Error: failure while waiting for child to complete.\n";
		
	signal(SIGINT, SIG_DFL);
	
	return 1;
}

// splits the token vector to a vector of vectors
// where each vector is what is between the pipes
vector < vector<const char *> > split_on_pipes(vector<const char *> args)
{
	vector< vector <const char * > > commands;
	commands.push_back(*(new vector<const char *>));
	for (int i = 0; i < args.size(); ++i)
	{
		if (strcmp(args[i], "|") == 0) // when it's a pipe
			commands.push_back(*(new vector<const char *>)); // start the next set of commands
		else
			commands.back().push_back(args[i]); // if it's not a pipe, just stick it in the last set of commands
	} 
			

	return commands;
}

// prints the prompt and gets the line of input
istream& getinput(string &l)
{
	//char temp[WD_MAXLEN];
	//getcwd(temp, WD_MAXLEN);
	//cout << temp << "$ ";
	
	cout << "$$ ";
	return getline(cin, l);
}

// prints out the arguments of the command vector passed in
void printargs(vector<const char *> argv)
{
	cout << "ARGS: ";
	for (int i = 0; i < argv.size() ; ++i)
		cout << argv[i] << " ";
	cout << endl;
}

// unneeded
void parent_sighand(int sig)
{
	
}

void child_sighand(int sig)
{
	exit(EXIT_FAILURE);
}