#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <stdlib.h>
#include <wait.h>
#include <unistd.h>
#include <limits.h>
#include <csignal>
#include <cerrno>


using namespace std;

void parse_args(const string& line, vector<string>& cmds) {
    stringstream liness(line);
    string token;
    while (getline(liness, token, ' ')) {
        if (!token.empty())
            cmds.push_back(token);
    }
}

volatile sig_atomic_t sigint_count = 0;
volatile sig_atomic_t sigtstp_count = 0;

void handle_sigint(int signum) {
    sigint_count++;
}

void handle_sigtstp(int signum) {
    sigtstp_count++;
}


int main(int argc, char* argv[]) {
    //history for storing last 10 commands
    vector<string> history;
    // Get the shell name
    string shell_name = (argc > 0) ? argv[0] : "Shell";
    //Total command count  
    int total_commands = 0;

    //Register signal handlers
    signal(SIGINT, handle_sigint);
    signal(SIGTSTP, handle_sigtstp);
    
    while (true) {
        // Get the current working directory
        char cwd[PATH_MAX];
        //handle getting the directory
        if (getcwd(cwd, sizeof(cwd)) == nullptr) {
            perror("getcwd error");
            continue;
        }

        // Create a command number tracker for the history
        int command_number = history.size() + 1;
        // Increment the total command count
        total_commands++;
        // Print the shell prompt with the current working directory
        cout << "[" << shell_name << "][" << cwd << "][#" << total_commands << "]$ ";
        // Handle user input
        string cmd;
        getline(cin, cmd);
        if (cmd.empty())
            continue;

        // Handle 'r' and 'r N' commands for history rerun
        if (!cmd.empty() && cmd[0] == 'r') {
            if (cmd == "r") {
                // Handle empty history for r command
                if (history.empty()) {
                    cerr << "Error: No command in history to rerun.\n";
                    continue;
                }
                //Use last command in history
                cmd = history.back();
                //write the command to console from the last history
                cout << "[" << shell_name << "][" << cwd << "][#" << total_commands << "]$ " << cmd << endl;
            }
            // Handle 'r N' where N is a number
             else if (cmd.size() > 2 && cmd[1] == ' ') {
                // Get the number after 'r'
                string num_str = cmd.substr(2);
                // Convert the string to an int and check if it's valid
                int num = atoi(num_str.c_str());
                if (num <= 0 || num > (int)history.size()) {
                    cerr << "No such command in history: " << num << endl;
                    continue;
                }
                //Get the command from history
                cmd = history[num - 1];
                //Display the command to console
                cout << "[" << shell_name << "][" << cwd << "][#" << total_commands << "]$ " << cmd << endl;
            } 
            // Display error for invalid 'r' usage
            else {
                cerr << "Invalid usage of r command\n";
                continue;
            }
        }

        // Store command in history (max 10)
        history.push_back(cmd);
        if (history.size() > 10)
            history.erase(history.begin());

        // Built-in commands
        if (cmd == "help") {
            cout << "Built-in commands:\n";
            cout << "  ls    - List files in the current directory\n";
            cout << "  ls -1 - List files in the current directory in a single column\n";
            cout << "  ls -a - List all files including hidden files\n";
            cout << "  help  - Show this help message\n";
            cout << "  exit  - Exit the shell\n";
            cout << "  hist  - Show last 10 commands\n";
            cout << "  pwd   - Print current working directory\n";
            cout << "  r     - Rerun the last command\n";
            cout << "  r N   - Rerun command number N from history\n";
            continue;
        } 
        //Exit entry
        else if (cmd == "exit") {
            cout << "Signal Summary before exiting:\n";
            cout << "SIGINT (Ctrl+C): " << sigint_count << " times\n";
            cout << "SIGTSTP (Ctrl+Z): " << sigtstp_count << " times\n";
            cout << "Exiting shell...\n";
            exit(0);
        } 
        // Handle hist
        else if (cmd == "hist") {
            cout << "Command History:\n";
            for (size_t i = 0; i < history.size(); i++) {
                cout << i + 1 << ": " << history[i] << endl;
            }
            continue;
        } 
        //Handl pwd
        else if (cmd == "pwd") {
            char cwd[PATH_MAX];
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                cout << cwd << endl;
            } else {
                perror("getcwd error");
            }
            continue;
        }
        //Break the command into words 
        vector<string> cmd_args;
        parse_args(cmd, cmd_args);
        if (cmd_args.empty())
            continue;

        // Prepare argv for execvp
        vector<char*> argv;
        for (size_t i = 0; i < cmd_args.size(); ++i) {
            argv.push_back(const_cast<char*>(cmd_args[i].c_str()));
        }
        argv.push_back(nullptr);
        //Fork a new process
        int pid = fork();
        if (pid == 0) {
            execvp(argv[0], argv.data()); // Try to run the command
            perror("execvp failed"); //fail error
            cout << "The program seems missing. Error code is: " << errno << endl;
            exit(1); //exit the child process 
        }
        //Parent process 
        else if (pid > 0) {
            int status;
            //Waid for the child process to finish
            waitpid(pid, &status, 0);
            if(WIFEXITED(status)) {
                int code = WEXITSTATUS(status);
                if(code != 0) {
                    cout << "[Program exited with error code: " << code << "]" << endl;
                }
            } else if (WIFSIGNALED(status)){
                cout << "[Program terminated by signal: " << WTERMSIG(status) << "]" << endl;
            }
        } 
        //Fork error
        else {
            perror("fork failed");
        }
    }

    return 0;
}
