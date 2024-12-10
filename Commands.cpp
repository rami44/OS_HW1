#include <unistd.h>
#include <string.h>
#include <iostream>
#include <iostream>   // For std::cout, std::cerr
#include <streambuf>  // For std::streambuf
#include <unistd.h>
#include <fcntl.h>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <string>
#include <sys/types.h>
#include <sys/syscall.h>
#include <regex>
//#include <dirent.h>
#include <unistd.h>
#include <cstring>
#include <algorithm>

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

bool isNumber(char *arg) {
    try {
        stoi(arg);
    } catch (...) {
        return false;
    }
    return true;
}


string _ltrim(const std::string &s) {
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string &s) {
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string &s) {
    return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char *cmd_line, char **args) {
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for (std::string s; iss >> s;) {
        args[i] = (char *) malloc(s.length() + 1);
        memset(args[i], 0, s.length() + 1);
        strcpy(args[i], s.c_str());
        args[++i] = NULL;
    }
    return i;

    FUNC_EXIT()
}

bool _isBackgroundComamnd(const char *cmd_line) {
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char *cmd_line) {
    const string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == string::npos) {
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&') {
        return;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    cmd_line[idx] = ' ';
    // truncate the command line string up to the last non-space character
    cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}    

// TODO: Add your implementation for classes in Commands.h
JobsList::JobEntry *JobsList::getJobById(int jobId)
{
    for(JobEntry* job : jobs_vector)
        if(job->getID() == jobId)
            return job;
    return nullptr;
}

void JobsList::removeJobById(int jobId)
{
    for(auto it= jobs_vector.begin(); it != jobs_vector.end(); ++it) {
        if ((*it)->getID()==jobId) {
            delete *it;
            jobs_vector.erase(it);
            return;
        }
    }
}

void JobsList::updateMaxJob() {
        int maxID = -1;
        for (auto& job : jobs_vector) {
            if (job->getID() > maxID) {
                maxID = job->getID();
                maxJob = job;
            }
        }
	}

void JobsList::killAllJobs() {
        for (auto& job : jobs_vector) {
            kill(job->getProcessId(), SIGKILL);
        }
        updateMaxJob();
 }
    
    

SmallShell* SmallShell::instance = nullptr;

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/


/////////////////////////////////PIPE/////////////////////////////////


//------------------------------PIPE--------------------------------//
void PipeCommand::trim(std::string& str) {
    size_t first = str.find_first_not_of(' ');
    size_t last = str.find_last_not_of(' ');
    if (first == std::string::npos || last == std::string::npos)
        str.clear();
    else
        str = str.substr(first, (last - first + 1));
}



 PipeCommand::PipeCommand(const char *cmd_line) : Command(cmd_line), isStderrRedirected(false) {
     std::string commandStr(cmd_line);
        size_t separatorPos = commandStr.find("|");
        if (separatorPos != std::string::npos) {
            leftcommand = commandStr.substr(0, separatorPos);
            rightcommand = commandStr.substr(separatorPos + 1);
            isStderrRedirected = (commandStr[separatorPos + 1] == '&');
            if (isStderrRedirected) {
                // Adjust rightCommand to start after "|&"
                rightcommand = commandStr.substr(separatorPos + 2);
            }
            trim(leftcommand);
            trim(rightcommand);
        }
    }

    

void  PipeCommand::execute()  {
        int pipefd[2];
        if (pipe(pipefd) == -1) {
            perror("smash error: pipe failed");
            return;
        }

        pid_t leftPid = fork();
        if (leftPid == 0) {
            // Left command runs in the child process
            close(pipefd[0]); // Close unused read end
            if (dup2(pipefd[1], isStderrRedirected ? STDERR_FILENO : STDOUT_FILENO) == -1) {
                perror("smash error: dup2 failed");
                exit(1);
            }
            close(pipefd[1]); // Close the write end after duplicating
            system(leftcommand.c_str()); // Execute the left command
            exit(0);
        } else if (leftPid < 0) {
            perror("smash error: fork failed");
            return;
        }

        pid_t rightPid = fork();
        if (rightPid == 0) {
            // Right command runs in the child process
            close(pipefd[1]); // Close unused write end
            if (dup2(pipefd[0], STDIN_FILENO) == -1) {
                perror("smash error: dup2 failed");
                exit(1);
            }
            close(pipefd[0]); // Close the read end after duplicating
            system(rightcommand.c_str()); // Execute the right command
            exit(0);
        } else if (rightPid < 0) {
            perror("smash error: fork failed");
            return;
        }

        // Parent closes both ends and waits for children
        close(pipefd[0]);
        close(pipefd[1]);
        waitpid(leftPid, NULL, 0);
        waitpid(rightPid, NULL, 0);
    }




/*
PipeCommand::PipeCommand(const char *cmd_line) : Command(cmd_line) {
	string str(cmd_line);
	
	int place;
	error_flag = false;
	if(_isBackgroundComamnd(str.c_str())) {
		char tmp[COMMAND_MAX_ARGS]{0};
		strcpy(tmp, cmd_line);
		_removeBackgroundSign(tmp);
		cmd_line = tmp;
	}
	if((place=str.find("|&")) != -1) {
		command1 = _trim(str.substr(0, place));
		command2 = _trim(str.substr(place+2,str.length()));
	}
	else {
		if((place=str.find("|")) != -1) {
			error_flag = true;
			command1 = _trim(str.substr(0, place));
			command2 = _trim(str.substr(place+1, str.length()));
		}
	}
}

void PipeCommand::execute() {
    char **args = new char *[COMMAND_MAX_ARGS]{0};
    _parseCommandLine(command, args);
    int my_pipe[2];
    pipe(my_pipe);
    pid_t pid1=fork();
    if (pid1 <0){
        if(close(my_pipe [0])==-1)
        {
            perror("smash error: close failed");
        }
        if(close(my_pipe [1])==-1)
        {
            perror("smash error: close failed");
        }
        perror("smash error: fork failed");
        delete [] args;
        return;
    }
    if(pid1==0){ //child 1
        if (setpgrp()==-1)
        {
            if(close(my_pipe [0])==-1)
            {
                perror("smash error: close failed");
            }
            if(close(my_pipe [1])==-1)
            {
                perror("smash error: close failed");
            }
            perror("smash error: setpgrp failed");
            delete [] args;
            return;
        }
        if(error_flag){ // if |
            if(close(my_pipe [0])==-1)
            {
                perror("smash error: close failed");
                delete [] args;
                return;
            }
            if (dup2(my_pipe[1],1)==-1){
                if(close(my_pipe [0])==-1)
                {
                    perror("smash error: close failed");
                }
                if(close(my_pipe [1])==-1)
                {
                    perror("smash error: close failed");
                }
                perror("smash error: dup2 failed");
                delete [] args;
                return;
            }
        }
        else{ // if |&

            if(close(my_pipe[0])==-1)
            {
                perror("smash error: close failed");
                delete [] args;
                return;
            }

            if (dup2(my_pipe[1],2)==-1){
                if(close(my_pipe [0])==-1)
                {
                    perror("smash error: close failed");
                }
                if(close(my_pipe [1])==-1)
                {
                    perror("smash error: close failed");
                }
                perror("smash error: dup2 failed");
                delete [] args;
                return;
            }
        }


        SmallShell  &shell =SmallShell ::getInstance();
        shell.executeCommand(command1.c_str());
        delete [] args;
        exit(0);
    }
    pid_t pid2= fork();
    if (pid2 <0){
        if(close(my_pipe [0])==-1)
        {
            perror("smash error: close failed");
        }
        if(close(my_pipe [1])==-1)
        {
            perror("smash error: close failed");
        }
        perror("smash error: fork failed");
        delete [] args;
        return;
    }
    if(pid2==0){ //child 2
        if (setpgrp()==-1)
        {
            if(close(my_pipe [0])==-1)
            {
                perror("smash error: close failed");
            }
            if(close(my_pipe [1])==-1)
            {
                perror("smash error: close failed");
            }
            perror("smash error: setpgrp failed");
            delete [] args;
            return;
        }
        if(close(my_pipe[1])==-1)
        {
            perror("smash error: close failed");
            delete [] args;
            return;
        }
        if( dup2(my_pipe[0],0)==-1){
            if(close(my_pipe [0])==-1)
            {
                perror("smash error: close failed");
            }
            perror("smash error: dup2 failed");
            delete [] args;
            return;
        }
        SmallShell  &shell =SmallShell ::getInstance();
        shell.executeCommand(command2.c_str());
        delete [] args;
        exit(0);
    }
    delete [] args;
    //closing the pipe for the parent
    if(close(my_pipe [0])==-1)
    {
        perror("smash error: close failed");
    }
    if(close(my_pipe [1])==-1)
    {
        perror("smash error: close failed");
    }
    if (waitpid(pid1,nullptr,WUNTRACED)==-1){
        perror("smash error: waitpid failed");
    }
    if (waitpid(pid2,nullptr,WUNTRACED)==-1){
        perror("smash error: waitpid failed");
        return;
    }
}

*/
/////////////////////////////END-PIPE/////////////////////////////////

std::string SmallShell::processBackgroundCommand(char* cmd) {
    std::string cmd_string = std::string(cmd);

    size_t amp_index = cmd_string.find_last_of('&');
    size_t non_space_index = cmd_string.find_last_not_of(' ', amp_index - 1);

    std::string rest = cmd_string.substr(non_space_index + 1); // Store trailing part
    _removeBackgroundSign(cmd); // Remove '&'

    return rest; // Return the rest string
}

Command* SmallShell::CreateCommand(const char* cmd_line) {
	    std::string remaining; // Handle background commands

    if (*cmd_line == '\0') {
        return nullptr; 
    }

    std::string alias_command;
    char* cmd = new char[COMMAND_MAX_ARGS];
    strcpy(cmd, cmd_line);

    
    if (_isBackgroundComamnd(cmd_line)) {
		remaining = processBackgroundCommand(cmd);
	}

    std::string cmd_s = _trim(std::string(cmd));
    char** args = new char*[COMMAND_MAX_ARGS]{nullptr}; // Initialize to nullptr
    int len = _parseCommandLine(cmd, args);
    bool is_Alias = false;

    std::string firstWord;
    if (isAlias(args[0])) {
        is_Alias = true;
        alias_command = args[0]; // Store alias name
        
        std::string command = aliases.find(args[0])->second; // Get actual command for alias

        char** inner_arguments = new char*[COMMAND_MAX_ARGS]{nullptr};
        int inner_len = _parseCommandLine(command.c_str(), inner_arguments);

        if (inner_len >= 2) {
            firstWord = inner_arguments[0];
            cmd_s = command; // Full alias command
        } else {
            firstWord = std::string(inner_arguments[0]);
            cmd_s = firstWord;
            for (int i = 1; i < len; i++) {
                alias_command += " ";
                cmd_s += " ";
                cmd_s += std::string(args[i]);
                alias_command += std::string(args[i]);
            }
        }

        for (int i = 0; i < inner_len; i++) {
            delete[] inner_arguments[i];
        }
    } 
    else {
        firstWord = std::string(cmd_line).substr(0, cmd_s.find_first_of(" \n"));
    }

    if (_isBackgroundComamnd(cmd_line)) {
        cmd_s += remaining; 
        alias_command += remaining;
    }

    // Allocate final command strings
    char* temp = new char[COMMAND_MAX_ARGS];
    char* final_alias = new char[COMMAND_MAX_ARGS];
    strcpy(temp, cmd_s.c_str());
    strcpy(final_alias, alias_command.c_str());
    
    
  if(strstr(cmd_line, "|") != nullptr || strstr(cmd_line, "|&") != nullptr){
     return new PipeCommand(temp);
  }
  
  if(strstr(cmd_line, ">") != nullptr || strstr(cmd_line, ">>") != nullptr){
     return new RedirectionCommand(temp);
  }
  if(firstWord == "alias"){
      return new aliasCommand(temp);
  }
  else if (firstWord == "unalias"){
	 return new unaliasCommand(temp);
  }
  else if(firstWord == "chprompt"){
      return new ChpromptCommand(temp);
  }
  else if(firstWord == "showpid"){
      return new ShowPidCommand(temp); //DONE
  }
  else if(firstWord == "pwd"){
      return new GetCurrDirCommand(temp); //DONE
  }
  else if(firstWord == "cd"){
      return new ChangeDirCommand(temp, SmallShell::getInstance().getLastPathAddress()); //DONE
  }
  else if(firstWord == "jobs"){
      return new JobsCommand(temp);  //WAIT
  }
  else if(firstWord == "fg") {
      return new ForegroundCommand(temp, jobs_list);  //80% DONE
  }
  else if(firstWord == "quit"){
      return new QuitCommand(temp, jobs_list); //DONE
  }
  else if(firstWord == "kill"){
      return new KillCommand(temp, jobs_list);  //DONE
  }
  else if(firstWord == "listdir"){
      return new ListDirCommand(temp);
  }
  else if(firstWord == "whoami"){
      return new WhoAmICommand(temp);
  }
  else{
    return new ExternalCommand(temp, is_Alias ? final_alias : nullptr);
  }

	for (int i = 0; i < len; i++) {
        delete[] args[i];
    }
	delete[] temp;
	delete[] final_alias;

  return nullptr;
}

/*
Command* SmallShell::CreateCommand(const char* cmd_line) {
	
	if(*cmd_line == '\0') { return nullptr; }
	
	
	
    // Trim and initialize command variables
    string cmd_s = _trim(string(cmd_line));
	string firstWord;
	string alias_command;

	char** arguments = new char*[COMMAND_MAX_ARGS]{0}; // Ensure array is initialized to nullptr
	int full_length = _parseCommandLine(cmd_line, arguments);

	// Check if the command is an alias
	auto it = aliases.find(cmd_s.substr(0, cmd_s.find(' '))); // Get the first word of the command
	if (it != aliases.end()) {
		std::string command = it->second; // Get the real command for the alias

		int actual_command_length = _parseCommandLine(command.c_str(), arguments);



		if (actual_command_length == 1) { 
			// Alias without additional arguments
			firstWord = command;
			cmd_s = firstWord;
		} else {
			// Alias with additional arguments
			alias_command = command; // Full alias command
			firstWord = command.substr(0, command.find(' ')); // Store just the alias name
			cmd_s = command;

			// Append additional arguments to the aliased command
			for (int i = 1; i < full_length; i++) {
				cmd_s += " ";
				cmd_s += std::string(arguments[i]);
			}
		}
	} 
	else {
		// Not an alias, extract the first word of the command
		firstWord = cmd_s.substr(0, cmd_s.find(' '));
	}

// Convert cmd_s to a dynamically allocated char*
	char* cmd = new char[cmd_s.length() + 1]; // +1 for null terminator
	strcpy(cmd, cmd_s.c_str()); // Copy the string

// Clean up dynamically allocated memory for arguments
	for (int i = 0; i < full_length; i++) {
		delete[] arguments[i];
	}
	
	if(strstr(cmd_line, "|") != nullptr || strstr(cmd_line, "|&") != nullptr){
      return new PipeCommand(cmd_line);
	}
    // Redirect to the appropriate Command object
    if (strstr(cmd_line, ">") != nullptr || strstr(cmd_line, ">>") != nullptr) {
        return new RedirectionCommand(cmd_line);
	}
	
	std::cout << firstWord << std::endl;
	
	if(firstWord=="alias") {
		return new aliasCommand(cmd);
    } else if (firstWord == "chprompt") {
        return new ChpromptCommand(cmd);
    } else if (firstWord == "showpid" || firstWord == "showpid&") {
        return new ShowPidCommand(cmd);
    } else if (firstWord == "pwd" || firstWord == "pwd&") {
        return new GetCurrDirCommand(cmd);
    } else if (firstWord == "cd") {
        return new ChangeDirCommand(cmd, SmallShell::getInstance().getLastPathAddress());
    } else if (firstWord == "jobs" || firstWord == "jobs&") {
        return new JobsCommand(cmd);
    } else if (firstWord == "fg") {
        return new ForegroundCommand(cmd, jobs_list);
    } else if (firstWord == "quit") {
        return new QuitCommand(cmd, jobs_list);
    } else if (firstWord == "kill") {
        return new KillCommand(cmd, jobs_list);
    } else if (firstWord == "whoami") {
        return new WhoAmICommand(cmd);
    } else if (firstWord == "listdir") {
        return new ListDirCommand(cmd);
    } else {
        return new ExternalCommand(cmd, nullptr);
    }
}
*/

ExternalCommand::ExternalCommand(const char* cmd_line, const char* alias) : alias(alias), Command(cmd_line) {}

void ExternalCommand::execute() {
    pid_t pid = fork();
    if (pid < 0) {
        perror("smash error: fork failed");
        return;
    }
	bool isBackground;
	
    char* tmp = new char[COMMAND_MAX_ARGS];
    strcpy(tmp, command);
    
    if (_isBackgroundComamnd(tmp)) {
		isBackground = true;
        _removeBackgroundSign(tmp); // Remove '&' for background commands
    }

    char bashDir[] = "/bin/bash";
    char flag[] = "-c";

    char** argv = new char*[COMMAND_MAX_LENGTH]{0};
    _parseCommandLine(tmp, argv);
   
    if (pid == 0) { // Child process
        if (setpgrp() == -1) { // Create a new process group
            perror("smash error: setpgrp failed");
            delete[] argv;
            free(tmp);
            exit(1);
        }
        
       
        // Check for complex commands (wildcards: * or ?)
        if (string(command).find('*') != string::npos || string(command).find('?') != string::npos) {
            char* complex_argv[] = {bashDir, flag, tmp, nullptr};
            if (execv(bashDir, complex_argv) == -1) {
                perror("smash error: execv failed");
                delete[] argv;
                free(tmp);
                exit(1);
            }
        } else { // Simple command
            if (execvp(argv[0], argv) == -1) { // result is printed to terminal
                perror("smash error: execvp failed");
                delete[] argv;
                free(tmp);
                exit(0);
            }

        }

    } else { // Parent process

      		std::string command_string = std::string(tmp);
			SmallShell& smash = SmallShell::getInstance();

        if (isBackground) {
           
           char* originalCommand = new char[COMMAND_MAX_ARGS];
            
           Command* realCommand = new ExternalCommand(
					alias ? alias : command, 
					nullptr
			);
			strcpy(originalCommand, alias ? alias : command);

			
            SmallShell& smash = SmallShell::getInstance();
            
            smash.getJobsList().addJob(realCommand, pid, originalCommand, false);
            
            delete[] argv;
            free(tmp);
            return;
                
        } 
        else { // isBackground, add Job
            smash.current_pid = pid; // Update current running PID
            int status = 0;

            if (waitpid(pid, &status, WUNTRACED) == -1) {
                perror("smash error: waitpid failed");
            }
            smash.current_pid = -1; // Reset current PID after wait
            
           }
        }
}
	
 
void SmallShell::executeCommand(const char *cmd_line) {
    // TODO: Add your implementation here
    // for example:
    // Command* cmd = CreateCommand(cmd_line);
    // cmd->execute();
    // Please note that you must fork smash process for some commands (e.g., external commands....)
  
	jobs_list->removeFinishedJobs();
    std::string temp = cmd_line;
            
    Command* command = CreateCommand(temp.c_str());
    if (command) {
        command->execute();
        delete command;
    }
}


void SmallShell::setPrompt(const std::string& prompt) {
    shell_prompt = prompt + "> ";
}

const std::string &SmallShell::getPrompt() const {
    return shell_prompt;
}


void SmallShell::setLastPath(const char* path){
    if (lastPath) {
            free(lastPath);
        }
        if (path) {
            lastPath = strdup(path);
        } else {
            lastPath = nullptr;
        }
}
char* SmallShell::getLastPath() const {
        return lastPath;
    }
JobsList& SmallShell::getJobsList() {
    return *jobs_list;
}


//-----------------------job's class funcs------------------------//

JobsList:: ~JobsList() {
    for (auto job : jobs_vector) delete job;
}
void JobsList:: addJob(Command* command, pid_t pid, const char* entryCommand, bool isStopped) {
	int newJobID = 1;
    if(maxJob)
    {
        newJobID = maxJob->getID() + 1;
    }
    SmallShell& smash = SmallShell::getInstance();
    removeFinishedJobs();
    JobEntry* newJob = new JobEntry(newJobID, command, pid, entryCommand);
    smash.getJobsList().jobs_vector.push_back(newJob);
    maxJob = newJob;
    newJob->set_isRunning(!isStopped);
    }





void JobsList::removeFinishedJobs() {
    if(jobs_vector.empty()){
        maxJob = nullptr;
        return;
    }
    int ret = 0;
    int status = 0;

    for(vector<JobEntry*>::iterator it = jobs_vector.begin(); it!=jobs_vector.end();it++)
    {
        ret=waitpid((*it)->getProcessId(),&status,WNOHANG);
        if(ret==(*it)->getProcessId())
        {
            jobs_vector.erase(it);
            --it;
        }
    }
    if (jobs_vector.empty()){
        maxJob = nullptr;
        return;
    }
    updateMaxJob();
}




void JobsList::printJobsList() {
    // sorting
    for (auto i = jobs_vector.begin(); i != jobs_vector.end(); ++i) {
        auto min_it = i;
        for (auto j = i + 1; j != jobs_vector.end(); ++j) {
            if ((*j)->getID() < (*min_it)->getID()) {
                min_it = j;
            }
        }
        if (min_it != i) {
            std::swap(*i, *min_it);
        }
    }

    // Printing the sorted list of jobs
    for (const auto& job : jobs_vector) {
        std::cout << "[" << job->getID() << "] " << job->getEntryCommand() << std::endl;
    }
}



// Command Constructor
Command::Command( const char *cmd_line) : command(cmd_line) {
  
}

// Virtual destructor
Command::~Command() {
    
}


/////////////////////SPECIAL COMMANDS ////////////////////////

RedirectionCommand::RedirectionCommand( const char *cmd_line) : Command(cmd_line)
 {
	std::vector<char *> arguments(COMMAND_MAX_LENGTH, nullptr);
	
	_parseCommandLine(command, arguments.data());
	
	cmd = new char[COMMAND_MAX_ARGS];
	output_file = new char[COMMAND_MAX_ARGS];
	
	string cmd_string = _trim(string(command));
	string subCommand = _trim(cmd_string.substr(0, cmd_string.find_first_of(">")));
	string file_string = _trim(cmd_string.substr(cmd_string.find_last_of(">")+ 1, cmd_string.length() - 1));
	
	strcpy(cmd, subCommand.c_str());
	strcpy(output_file, file_string.c_str());
	
	if(cmd_string.find(">>") != string::npos) {
		override = false;
	}
	else {
		override = true;
	}
}

void RedirectionCommand::execute() {
	SmallShell& smash = SmallShell::getInstance();
		
	int original_stdout = dup(STDOUT_FILENO);

	int fd;
    if (!override) {
		fd = open(output_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
    } else {
        fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    }

        if (fd == -1) {
            perror("smash error: failed to open output file");
            exit(1);
        }

        // Redirect the child's stdout to the file
        if (dup2(fd, STDOUT_FILENO) == -1) {
            perror("smash error: failed to redirect stdout");
            close(fd);
            exit(1);
        }

        close(fd); // Close the file descriptor
      
  		smash.executeCommand(cmd);
			
		// Restore stdout to its original state (terminal)
		if (dup2(original_stdout, STDOUT_FILENO) == -1) {
        perror("Failed to restore stdout");
                    exit(1);

		}
		close(original_stdout); // The original stdout file descriptor is no longer needed
	}
		


//----------------------Build_In_Commands---------------------------//




//-------------------chprompt command (warm-up)----------------------//
ChpromptCommand::ChpromptCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}



void ChpromptCommand::execute() {
   // std::cout << "Executing ChpromptCommand" << std::endl;
   
    char **args=new char* [COMMAND_MAX_ARGS];
    	
    char* tmp = strdup(command); // Duplicate the command string

	_removeBackgroundSign(tmp);
	
	int numArgs = _parseCommandLine(tmp, args);
	   
    if (numArgs == 1) {
        //  no arguments default
        SmallShell::getInstance().setPrompt("smash");
    } else {
        // Set new prompt to the first argument
        SmallShell::getInstance().setPrompt(args[1]);
      
    }


    for (int i = 0; i < numArgs; ++i) {
       delete(args[i]);
    }
}

//------------------------showpid command---------------------//



ShowPidCommand::ShowPidCommand(const char *cmd_line):BuiltInCommand(cmd_line){};

void ShowPidCommand::execute() {
    std::cout << "smash pid is " << getpid() << std::endl;
}

//--------------------------pwd command------------------------//


GetCurrDirCommand::GetCurrDirCommand(const char *cmd_line):BuiltInCommand(cmd_line){}

void GetCurrDirCommand::execute(){
    char cwd[COMMAND_MAX_LENGTH];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
            std::cout << cwd << std::endl;
        } else {
            perror("smash error: getcwd failed");
        }
    
}

//--------------------Change Directory Command---------------------//

ChangeDirCommand::ChangeDirCommand(const char *cmd_line, char** lastPath) : BuiltInCommand(cmd_line), lastPath(lastPath) {}

void ChangeDirCommand::execute()  {
        char* args[COMMAND_MAX_ARGS];
        
        char* tmp = strdup(command); // Duplicate the command string

		_removeBackgroundSign(tmp);
	
		int numArgs = _parseCommandLine(tmp, args);

        if (numArgs > 2) {
            std::cerr << "smash error: cd: too many arguments" << std::endl;
        } else {
            const char* newPath = (numArgs == 1 ? "." : args[1]);

            if (strcmp(newPath, "-") == 0) {
                if (*lastPath == nullptr || std::string(*lastPath).empty()) {
                    std::cerr << "smash error: cd: OLDPWD not set" << std::endl;
                } else {
                    newPath = *lastPath;
                }
            }

            char currentPath[1024];
            if (getcwd(currentPath, sizeof(currentPath)) != nullptr) {
                if (chdir(newPath) == 0) {
                    *lastPath = strdup(currentPath);  //update
                }
                
            }
        }

        for (int i = 0; i < numArgs; ++i) {
            free(args[i]);
        }
    }

//-----------------------jobs command---------------------------//

JobsCommand::JobsCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}
void JobsCommand:: execute()  {
    SmallShell& shell = SmallShell::getInstance();
        shell.getJobsList().removeFinishedJobs();
        shell.getJobsList().printJobsList();
    }

//--------------------------fg command --------------------------//

ForegroundCommand::ForegroundCommand(const char* cmd_line, JobsList* jobs) 
    : BuiltInCommand(cmd_line), jobs(jobs) {}
    
void ForegroundCommand::execute()  {

	char **args= new char* [COMMAND_MAX_LENGTH]{0};
		
    char* tmp = strdup(command); // Duplicate the command string

	_removeBackgroundSign(tmp);
	
	int num_of_arguments = _parseCommandLine(tmp, args);


    // synteax error: if number of arguments or format is wrong print an error message
    if (num_of_arguments > 2 || num_of_arguments < 1) {
        std::cerr << "smash error: fg: invalid arguments" << std::endl;
        return;
    }

    // if joblist is empty, print an error message
    if (jobs->jobs_vector.empty()) {
        std::cerr << "smash error: fg: jobs list is empty" << std::endl;
        delete[] args;
        return;
    }

    int job_id;

    // If the job-id argument is not specified, then the job with maximal job-id is the job to be brought to the foreground
    if (num_of_arguments == 1) {
		jobs->updateMaxJob();
        job_id = jobs->getMaxJob()->getID();
    } else {
        try {
            job_id = std::stoi(args[1]);
        } catch (const std::invalid_argument &e) {
            std::cerr << "smash error: fg: invalid job-id" << std::endl;
            return;
        }
    }

    // if the job-id is not found in the job list, print an error message
    if (jobs->getJobById(job_id) == nullptr) {
        std::cerr << "smash error: fg: job-id " << job_id << " does not exist" << std::endl;
        return;
    }


    JobsList::JobEntry *job = jobs->getJobById(job_id); // get desired job object

    pid_t pid = job->getProcessId(); // get the process id of the job

    // a job is stopped and needs to be resumed before it can be brought to the foreground

    if (!job->get_isRunning()) { // If the job is stopped
        kill(pid, SIGCONT);
        job->set_isRunning(true);
    }

    // Shell status Update
    SmallShell &shell = SmallShell::getInstance();
    shell.current_pid = pid;
    shell.current_job_id = job_id;

	std::string Mycommand = job->getEntryCommand();
    // Print the command line of the job along with its process id
    std::cout << Mycommand << " " << pid << std::endl;
    
	jobs->removeJobById(job_id);

    delete[] args;

}

QuitCommand::QuitCommand(const char *cmd_line, JobsList *jobs) : 
		BuiltInCommand(cmd_line), jobs(jobs) {}

void QuitCommand::execute() {
    // quit command exits the smash. Only if the kill argument was specified (which is optional),
    // then smash should kill (By SIGKILL) all the unfinished jobs before exiting.
    // and print the number of processes/jobs that were killed. their PIDs and command lines.
    // You may assume that the kill argument, if present, will always be the first argument of the command.

    std::vector<char *> arguments(COMMAND_MAX_LENGTH, nullptr); // Vector for managing arguments

	char* tmp = strdup(command); // Duplicate the command string

	_removeBackgroundSign(tmp);
	
	int num_of_arguments = _parseCommandLine(tmp, arguments.data());

    // if any number of arguments (other than kill) is given, ignore them.

    if (num_of_arguments >= 2) {
        string second_argument = arguments[1];
        if (second_argument != "kill") {
            exit(0);
        } else {
            // get jobs_list in smallshell class
            vector<JobsList::JobEntry *> deleting_jobs = jobs->jobs_vector;

            int num_of_jobs = deleting_jobs.size();

            if (num_of_jobs == 0) { // no jobs to kill
               cout << "smash: sending SIGKILL signal to " << num_of_jobs << " jobs:" << endl;
               exit(0);
            }
            

            cout << "smash: sending SIGKILL signal to " << num_of_jobs << " jobs:" << endl;

            for (auto &job: deleting_jobs) {
                cout << job->getProcessId() << ": " << job->getEntryCommand() << endl;
            }

            jobs->killAllJobs();
        }
        exit(0);
    }
}

// KillCommand 8

KillCommand::KillCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), jobs(jobs) {}

void KillCommand::execute() {

    // command format: kill -<signum> <job-id>

    std::vector<char *> arguments(COMMAND_MAX_LENGTH, nullptr);
    
    char* tmp = strdup(command); // Duplicate the command string

	_removeBackgroundSign(tmp);
	
    int num_of_arguments = _parseCommandLine(tmp, arguments.data());

    // syntax error
    bool arg2_isnumber = isNumber(arguments[1]);
    if (num_of_arguments != 3 || !arg2_isnumber) {
        std::cerr << "smash error: kill: invalid arguments" << std::endl;
        return;
    }

    // job id does not exist
    int job_id = std::stoi(arguments[2]);
    JobsList::JobEntry *job = jobs->getJobById(job_id);
    if (job == nullptr) {
        std::cerr << "smash error: kill: job-id " << job_id << " does not exist" << std::endl;
        return;
    }


    SmallShell &smash = SmallShell::getInstance();

    JobsList::JobEntry *receiving_Job = jobs->getJobById(job_id);
    pid_t jobProcessID = receiving_Job->getProcessId();

    // get signal number to become negative
    int signum = std::stoi(arguments[1]);
    
    signum *= (-1);

    // SIGCONT and SIGSTOP
    switch (signum) {
        case SIGCONT: // If the signal is SIGCONT (continue)
            if (!receiving_Job->get_isRunning()) { // If the job is stopped
                smash.current_job_id = job_id;
                smash.current_pid = jobProcessID;
                jobs->removeJobById(job_id); // Remove job from the list as it's resumed
            }
            // If the job is not stopped, do nothing
            break;

        case SIGSTOP: // If the signal is SIGSTOP (stop)
            // Reset shell's current process and job
            smash.current_job_id = -1;
            smash.current_pid = -1;
            receiving_Job->set_isRunning(false); // Mark job as stopped
            break;
        default:
            break; // other signals don't need shell status update
    }
    	jobs->removeFinishedJobs();

    
    if (kill(jobProcessID, signum) == 0) { // If `kill` was successful

        cout << "signal number " << signum << " was sent to pid " << jobProcessID << endl;
    } else { // If `kill` failed
		
        perror("smash error: kill failed"); // Print error if `kill` fails
        return; // Exit the function
    }
}


// whoamI

WhoAmICommand::WhoAmICommand(const char* cmd_line): Command(cmd_line) {}

void WhoAmICommand::execute() {
    uid_t uid = getuid(); // Get the current user's UID
    
    
    int fd = open("/etc/passwd", O_RDONLY);
    if (fd < 0) {
        const char* error_msg = "smash error: open failed\n";
        write(STDERR_FILENO, error_msg, strlen(error_msg));
        return;
    }

    char buffer[120000];
    ssize_t bytes_read;
    std::string line;
    std::string username, home_dir;

    // Read and process /etc/passwd
    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
        for (ssize_t i = 0; i < bytes_read; ++i) {
            if (buffer[i] == '\n') {
                // Parse the line when a full line is read
                                
                size_t user_end = line.find(':');  // Find the end of the username
               
                size_t uid_start = line.find(':', user_end + 1) + 1;  // Start of the UID
                size_t uid_end = line.find(':', uid_start);  // End of the UID
               
				size_t x = line.find(':', uid_end+1);
                
                size_t home_start = line.find(':',  x + 1) + 1;  // Start of the home directory
                size_t home_end = line.find(':', home_start + 1);  // End of the home directory

                // Ensure the line is correctly parsed
                if (user_end != std::string::npos && uid_start != std::string::npos && uid_end != std::string::npos && home_start != std::string::npos) {
                    uid_t line_uid = std::stoi(line.substr(uid_start, uid_end - uid_start));  // Extract UID
                    
                 // Check if the UID matches the current user's UID
                    if (line_uid == uid) {
                        username = line.substr(0, user_end);  // Extract username
                        home_dir = line.substr(home_start, home_end - home_start);  // Extract home directory
                        break;  // Found the matching user, break out of the loop
                    }
                }

                line.clear();  // Reset line for the next read
            } else {
                line += buffer[i];  // Append the current character to the line
            }
        }

        if (!username.empty()) {
            break;  // Exit if the username was found
        }
    }

    close(fd);

    // If no username was found, output error
    if (username.empty()) {
        write(STDERR_FILENO, "smash error: user not found\n", 28);
        return;
    }
    
    // Output the username and home directory
    std::string output = username + " " + home_dir + "\n";
    write(STDOUT_FILENO, output.c_str(), output.length());
}


////////////////////////LISTDIR////////////////////////////////////////


ListDirCommand::ListDirCommand(const char *cmd_line) : Command(cmd_line) {}


struct linux_dirent
{
    unsigned long d_ino;
    off_t d_off;
    unsigned short d_reclen;
    char d_name[];
};

void sort_vectors(vector<string>& vec) {
    sort(vec.begin(), vec.end());
}

void list_directory_recursive(const string& path, int depth) {
    const size_t BUF_SIZE = 4096;
    char buf[BUF_SIZE];
    
    struct linux_dirent *d;
    
    int fd;
    fd = open(path.c_str(), O_RDONLY | O_DIRECTORY);

    if (fd == -1) {
        perror(("smash error: open failed for " + path).c_str());
        return;
    }

    vector<string> files;
    vector<string> directories;

    // Read directory entries
    int nread;
    while ((nread = syscall(SYS_getdents, fd, buf, BUF_SIZE)) > 0) {
        for (int bpos = 0; bpos < nread;) {
            d = (struct linux_dirent *)(buf + bpos);
            char d_type = *(buf + bpos + d->d_reclen - 1);
            const string name = d->d_name;

            if (!name.empty() && name[0] != '.') {
                if (d_type == 8) {
                    files.push_back(name);  // Regular file
                } else if (d_type == 4) {
                    directories.push_back(name);  // Directory
                }
            }

            bpos += d->d_reclen;  // Move to the next directory entry
        }
    }

    if (nread == -1) {
        close(fd);
        perror("smash error: getdents failed");
        return;
    }

    close(fd);

    // Sort files and directories alphabetically
    sort_vectors(files);
    sort_vectors(directories);

    // Print directories first with tabs
    for (const string& dir_name : directories) {
        cout << string(depth, '\t') << dir_name << endl; 
        list_directory_recursive(path + "/" + dir_name, depth + 1);  // recursion
    }

    // Print files with tabs
    for (const string& file_name : files) {
        cout << string(depth, '\t') << file_name << endl; 
    }
}

void ListDirCommand::execute() {
	
    vector<char*> arguments(COMMAND_MAX_LENGTH, nullptr);
    
    char* tmp = strdup(command);
    _removeBackgroundSign(tmp);
    
    if (_parseCommandLine(tmp, arguments.data()) > 2) {
        cerr << "smash error: listdir: too many arguments" << endl;
        free(tmp);
        return;
    }
	string path;
	
	if(_parseCommandLine(tmp, arguments.data()) == 2) {
		path = arguments[1];
	}
	else {
		path = ".";
	}

	// using recursion to display files and directories in hierarchial format
    list_directory_recursive(path, 0);

    free(tmp); 
}

//////////////////END OF LISTDIR//////////////////////////////////////////




/////////////////////ALIAS//////////////////////////////////////////////

bool validAlias(const string& command){
    const regex pattern("^alias [a-zA-Z0-9_]+='[^']*'$");
    return regex_match(command, pattern);
}

bool reservedAlias(const string& alias){
    SmallShell& shell = SmallShell::getInstance();

    if((alias == "chprompt") || (alias == "showpid") || (alias == "pwd") || (alias == "cd") || (alias == "jobs") ||
            (alias == "fg") || (alias == "quit") || (alias == "kill") || (alias == "alias") || (alias == "unalias") ||
            (alias == ">") || (alias == ">>") || (alias == "|") || (alias == "listdir") || (alias == "getuser") || (alias == "watch")){
        return true;
    }
    return shell.aliases.find(alias) != shell.aliases.end();
}


aliasCommand::aliasCommand(const char *cmd_line): BuiltInCommand(cmd_line) { }

void aliasCommand::execute() {
    SmallShell& shell = SmallShell::getInstance();
    char** args = new char*[COMMAND_MAX_ARGS];
    int num_args = _parseCommandLine(command, args); // Parse the command line arguments

    // Case 1: No arguments – print all aliases
    if (num_args == 1) {
        for (const auto& alias_entry : shell.insertion_order) {
            std::cout << alias_entry << " = '" << shell.aliases[alias_entry] << "'" << std::endl;
        }
        delete[] args;
        return;
    }

    // Extract alias and original command from the input
    std::string cmd_str = _trim(command);
    size_t equal_pos = cmd_str.find('=');

    if (equal_pos == std::string::npos) {
        std::cerr << "smash error: alias: invalid format" << std::endl;
        delete[] args;
        return;
    }

    // Extract alias name
    std::string alias_name = _trim(cmd_str.substr(5, equal_pos - 5));

    // Extract original command (removing surrounding single quotes if present)
    std::string original_command = _trim(cmd_str.substr(equal_pos + 1));
    if (original_command.front() == '\'' && original_command.back() == '\'') {
        original_command = original_command.substr(1, original_command.size() - 2);
    }

    // Validation: Check for reserved aliases
    if (reservedAlias(alias_name)) {
        std::cerr << "smash error: alias: '" << alias_name << "' is reserved or already exists" << std::endl;
        delete[] args;
        return;
    }

    // Validation: Check for valid alias format
    if (!validAlias(cmd_str)) {
        std::cerr << "smash error: alias: invalid alias name" << std::endl;
        delete[] args;
        return;
    }
    // Add or update the alias
    

    shell.aliases[alias_name] = original_command;
    shell.insertion_order.push_back(alias_name);
    
    delete[] args; // Free dynamically allocated memory
}




// UNALIAS



unaliasCommand::unaliasCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void unaliasCommand::execute() {
    SmallShell& shell = SmallShell::getInstance();
    char** args = new char*[COMMAND_MAX_ARGS]{nullptr};
    int n = _parseCommandLine(command, args);

    if (n <= 1) {
        cerr << "smash error: unalias: not enough arguments" << endl;
	}     
    for (int i = 1; i < n; ++i) {
		    std::string alias_name(args[i]);
            if (!shell.isAlias(alias_name)) {
                cerr << "smash error: unalias: " << alias_name << " alias does not exist" << endl;
            } 
            else { // isALias
                shell.aliases.erase(alias_name);
                auto& order = shell.insertion_order;
                order.erase(std::remove(order.begin(), order.end(), alias_name), order.end());
            }
        }
    for (int i = 0; i < n; ++i) {
        delete[] args[i];
    }
    delete[] args;
}








