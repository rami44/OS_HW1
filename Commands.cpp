#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <string>

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

//------------------Small Shell Functions------------------------//

 Command *SmallShell::CreateCommand(const char *cmd_line) {
 
        string cmd_s = _trim(string(cmd_line));
        
        string first = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

        if (first == "chprompt") {
            return new ChpromptCommand(cmd_line);
        }
        else if (first=="showpid" || first=="showpid&"){ 
            return new ShowPidCommand(cmd_line);
        }
        else if(first=="pwd" || first=="pwd&"){
            return new GetCurrDirCommand(cmd_line);
        }
        else if (first=="cd"){
            return new ChangeDirCommand(cmd_line,SmallShell::getInstance().getLastPathAddress());
        }
        else if (first=="jobs" || first=="jobs&"){
            return new JobsCommand(cmd_line);
		}
		else if (first=="fg"){
            return new ForegroundCommand(cmd_line, jobs_list);
		}
		else if (first=="quit") {
			return new QuitCommand(cmd_line, jobs_list);
        }
        else if (first=="kill") {
			return new KillCommand(cmd_line, jobs_list);
		}
		
		else {
			return new ExternalCommand(cmd_line);
		}
           
    return nullptr;
}


ExternalCommand::ExternalCommand(const char* cmd_line) : Command(cmd_line) {}

void ExternalCommand::execute() {

    pid_t pid = fork();
    if (pid < 0) {
        perror("smash error: fork failed");
        return;
    }
	bool isBackground;
	
    char* tmp = strdup(command); // Duplicate the command string
    
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
            if (execvp(argv[0], argv) == -1) {
                perror("smash error: execvp failed");
                delete[] argv;
                free(tmp);
                exit(1);
            }
        }

    } else { // Parent process
        SmallShell& smash = SmallShell::getInstance();
        
        std::string command_string = std::string(tmp);
        
        if (!isBackground) {

            smash.current_pid = pid; // Update current running PID
            int status = 0;

            if (waitpid(pid, &status, WUNTRACED) == -1) {
                perror("smash error: waitpid failed");
            }
            smash.current_pid = -1; // Reset current PID after wait
        } else { // isBackground, add Job
            smash.getJobsList().addJob(command_string, pid);
        }
    }

    delete[] argv;
    free(tmp);

}
	
 
void SmallShell::executeCommand(const char *cmd_line) {
    // TODO: Add your implementation here
    // for example:
    // Command* cmd = CreateCommand(cmd_line);
    // cmd->execute();
    // Please note that you must fork smash process for some commands (e.g., external commands....)
  
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
void JobsList:: addJob(const std::string& command, pid_t pid) {
		removeFinishedJobs();
        jobs_vector.push_back(new JobEntry(nextJobID++, command, pid));
    }
void JobsList::removeFinishedJobs() {
        
    
   
        
        for (auto it = jobs_vector.begin(); it != jobs_vector.end(); ) {
            
            if (!(*it)->get_isRunning()) {
                
                delete *it;
                it = jobs_vector.erase(it);
            } else {
                ++it;  // Move to the next job if current one is still running
            }
        }
    


     
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
    if (num_of_arguments > 2 || num_of_arguments < 1 || !isNumber(args[1])) {
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
    jobs->removeJobById(job_id);

    // Shell status Update
    SmallShell &shell = SmallShell::getInstance();
    shell.current_pid = pid;
    shell.current_job_id = job_id;

	std::string Mycommand = job->getEntryCommand();
    // Print the command line of the job along with its process id
    std::cout << Mycommand << " " << pid << std::endl;
    
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
    
    if (kill(jobProcessID, signum) == 0) { // If `kill` was successful

        cout << "signal number " << signum << " was sent to pid " << jobProcessID << endl;
    } else { // If `kill` failed
		
        perror("smash error: kill failed"); // Print error if `kill` fails
        return; // Exit the function
    }
}


	


