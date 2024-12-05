#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlCHandler(int sig_num) {

    SmallShell& smash = SmallShell::getInstance();

    cout << "smash: got ctrl-C" << endl;
    
    if (smash.current_pid!= -1) {
        pid_t fgProcess = smash.current_pid;
           
        if (kill(fgProcess, SIGKILL) == 0) {
             cout << "smash: process " << fgProcess << " was killed" << endl;
        } else {
             perror("smash error: kill failed");
        }
        smash.current_pid = -1;
        smash.current_job_id = -1;
    }
}
