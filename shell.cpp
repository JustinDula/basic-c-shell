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
vector<const char *> parse_to_vector(string line);
int start_full_command (vector< const char * > args);
void printargs(vector<const char *> a);
void child_sighand(int sig);
void parent_sighand(int sig);
vector < vector<const char *> > split_on_pipes(vector<const char *> args);
int spawn_single_command(int i_ID, int o_ID, vector<const char *> argv);
int fork_commands(vector < vector < const char * > > commands);
int execvec(vector<const char *> argv);

int main(int argc, char ** argv)
{

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

int execvec(vector<const char *> argv)
{
	//cout << "E"; printargs(argv);
	if (execvp(argv[0], (char**) argv.data()) == -1)
	{
			cerr << "Error: execvp failed.\n";
			return -1;
	}
	return 0;
}

int spawn_single_command(int i_ID, int o_ID, vector<const char *> argv)
{
	pid_t pid = fork();
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

int fork_commands(vector < vector < const char * > > commands)
{
	pid_t pid;
	int i_ID = 0, fd[2];
	
	int i;
	for (i = 0; i < commands.size() - 1; ++i)
	{
		pipe(fd);
		spawn_single_command(i_ID, fd[1], commands[i]);
		close(fd[1]);
		i_ID = fd[0];
	}
	
	if (i_ID) // if it's not zero
		dup2 (i_ID, 0);
	
	return execvec(commands[i]);
}

int start_full_command (vector< const char * > args)
{
	pid_t pid;
	int status;
	
	signal(SIGUSR1, parent_sighand);

	pid = fork();
	if (pid == 0)
	{
		// child process
		vector < vector < const char * > > commands = split_on_pipes(args);
		
		//for (unsigned int i = 0; i < commands.size(); ++i) printargs(commands[i]);
		
		signal(SIGUSR1, child_sighand); 
		fork_commands(commands);
		
		exit(EXIT_FAILURE);
	} 
	else if (pid < 0)
	{
		// fork failed
		cerr << "Error: Fork failed.\n";
	}
	
	if ((pid =waitpid(pid, &status, 0)) < 0)
		cerr << "Error: failure while waiting for child to complete.\n";
		
	signal(SIGINT, SIG_DFL);
	
	return 1;
}

vector < vector<const char *> > split_on_pipes(vector<const char *> args)
{
	vector< vector <const char * > > commands;
	commands.push_back(*(new vector<const char *>));
	for (int i = 0; i < args.size(); ++i)
	{
		if (strcmp(args[i], "|") == 0)
			commands.push_back(*(new vector<const char *>));
		else
			commands.back().push_back(args[i]);
	}
			

	return commands;
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