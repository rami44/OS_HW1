#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <string>
#include <map>

#define COMMAND_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)


class Command {
protected:
    const char* command;
    // TODO: Add your data members
public:
    Command(const char *cmd_line);
    const char* getCommand() const {return command;};
    virtual ~Command();

    virtual void execute() = 0;

    //virtual void prepare();
    //virtual void cleanup();
    // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
public:
    BuiltInCommand(const char *cmd_line) :Command(cmd_line){};

    virtual ~BuiltInCommand() {
    }
    
    virtual void execute() override {
            //  specific to each built-in commands
        }
};

class ChpromptCommand : public BuiltInCommand {
public:

    ChpromptCommand(const char* cmd_line);

    virtual ~ChpromptCommand() {};

    virtual void execute() override;
};


class ExternalCommand : public Command {
	private:
		const char* alias;
	
public:
    ExternalCommand(const char *cmd_line, const char* alias);

    virtual ~ExternalCommand() {
    }

    void execute() override;
};


class PipeCommand : public Command {

    std::string leftCommand;

        std::string rightCommand;

        bool redirectStderr;

    // TODO: Add your data members

public:

    PipeCommand(const char *cmd_line);

   

    static void trim(std::string& str);



    virtual ~PipeCommand() {}



    void execute() override;

};




class RedirectionCommand : public Command {
    // TODO: Add your data members
    char* output_file;
    char* cmd;
    bool override;
public:
    explicit RedirectionCommand(const char *cmd_line);

    virtual ~RedirectionCommand() {}
    
    void RedirectionCommandAux(std::ofstream& output);

    void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
private:
    char** lastPath;

public:
    // TODO: Add your data members public:
    ChangeDirCommand(const char *cmd_line, char **plastPwd);

    virtual ~ChangeDirCommand() {
    }

    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
public:
    GetCurrDirCommand(const char *cmd_line);

    virtual ~GetCurrDirCommand() {
    }

    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
public:
    ShowPidCommand(const char *cmd_line);

    virtual ~ShowPidCommand() {
    }

    void execute() override;
};

class JobsList;

class QuitCommand : public BuiltInCommand {
	JobsList *jobs;
public:
    // TODO: Add your data members public:
    QuitCommand(const char *cmd_line, JobsList *jobs);

    virtual ~QuitCommand() {}

    void execute() override;
};


class JobsList {
public:
    class JobEntry {
        int jobId;
        Command* job_command;
        pid_t processId;

        std::string old_command;
        bool isRunning;
        
    public:
        JobEntry(int id, Command* cmd, pid_t pid, const char* old_command):
        jobId(id), job_command(cmd), processId(pid) ,old_command(old_command)  {}
        
        int getID() const { return jobId; }
        pid_t getProcessId() { return processId; }
        std::string getEntryCommand() const { return old_command; }
        Command* getOldCommand() const { return job_command; }
                bool get_isRunning() const { return isRunning; }
                void set_isRunning(bool status) { isRunning = status; }
                void finish() { isRunning = false; }
        
        // TODO: Add your data members
    };
//job class's external data members
		std::vector<JobEntry*> jobs_vector;
        JobEntry *maxJob;

        int nextJobID = 1;
    
    // TODO: Add your data members
public:
    JobsList(){};

    ~JobsList();

   void addJob(Command* command, pid_t pid, const char* entryCommand, bool isStopped);

    void printJobsList();

    void removeFinishedJobs();
    
    JobEntry *getJobById(int jobId);

    void removeJobById(int jobId);

    JobEntry *getLastJob(int *lastJobId);

    JobEntry *getLastStoppedJob(int *jobId);
    
    JobEntry* getMaxJob() { return maxJob; }
    
    void updateMaxJob();
    
    void killAllJobs();

    // TODO: Add extra methods or modify exisitng ones as needed
};

class JobsCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    JobsCommand(const char *cmd_line);

    virtual ~JobsCommand() {
    }

    void execute() override;
};

class KillCommand : public BuiltInCommand {
    // TODO: Add your data members
    JobsList *jobs;
public:
    KillCommand(const char *cmd_line, JobsList *jobs);

    virtual ~KillCommand() {
    }

    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    // TODO: Add your data members
     JobsList* jobs;

public:
    ForegroundCommand(const char *cmd_line, JobsList* jobs);

    virtual ~ForegroundCommand() {
    }

    void execute() override;
};

class ListDirCommand : public Command {
public:
    ListDirCommand(const char *cmd_line);

    virtual ~ListDirCommand() {
    }

    void execute() override;
};

class WhoAmICommand : public Command {
public:
    WhoAmICommand(const char *cmd_line);

    virtual ~WhoAmICommand() {
    }

    void execute() override;
};

class NetInfo : public Command {
    // TODO: Add your data members
public:
    NetInfo(const char *cmd_line);

    virtual ~NetInfo() {
    }

    void execute() override;
};

class aliasCommand : public BuiltInCommand {
public:
    aliasCommand(const char *cmd_line);

    virtual ~aliasCommand() {
    }

    void execute() override;
};

class unaliasCommand : public BuiltInCommand {
public:
    unaliasCommand(const char *cmd_line);

    virtual ~unaliasCommand() {
    }

    void execute() override;
};


class SmallShell {
private:
    // TODO: Add your data members
    
    static SmallShell* instance;
    
    std::string shell_prompt;

    JobsList* jobs_list;
    
    char* lastPath;
       
	
    SmallShell() {
        shell_prompt="smash> ";
        jobs_list=new JobsList();
        lastPath=nullptr;
        
    } // singleton (private constructor)

public:


    std::map<std::string,std::string> aliases;

	std::vector<std::string> insertion_order;
	
	pid_t smash_pid;
	pid_t current_pid;
    int current_job_id;
    
    
    bool isAlias(const std::string& alias) const {
		return aliases.find(alias) != aliases.end();
	}
	
	std::string processBackgroundCommand(char* cmd);

    Command *CreateCommand(const char *cmd_line);

    SmallShell(SmallShell const &) = delete; // disable copy ctor
    void operator=(SmallShell const &) = delete; // disable = operator
    static SmallShell &getInstance() // make SmallShell singleton
    {
        if (!instance)
            instance = new SmallShell();
                return *instance;
    }

    ~SmallShell();

    void executeCommand(const char *cmd_line);

    // TODO: add extra methods as needed

    void setPrompt(const std::string& prompt); // set prompt

    const std::string &getPrompt() const; // get prompt
    void setLastPath(const char* path);
    char* getLastPath() const;
    char** getLastPathAddress() {
        return &lastPath;
    }
    
    JobsList& getJobsList();
        
    
};


class NetInfoCommand : public Command {

public:

    NetInfoCommand(const char* cmd_line);

    virtual ~NetInfoCommand() {}

    void execute() override;

};




#endif //SMASH_COMMAND_H_
