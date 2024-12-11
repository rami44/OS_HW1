#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlCHandler(int sig_num) {
	
    SmallShell &smash = SmallShell::getInstance();
    
    std::cout << "smash: got ctrl-C" << std::endl;

    if(smash.current_pid != -1)
    {
        cout << "smash: process " << smash.current_pid <<" was killed" << endl;
        if(kill(smash.current_pid,SIGKILL)){
            perror("smash error: kill failed");
            return;
        }

        smash.current_pid = -1;
        smash.current_job_id = -1;
    }
    return;
}


