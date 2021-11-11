#include <vector>
#include <string>
#include <string.h>
#include <sstream>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>

// Common Definations
#define MAXLINE 80  // 80 characters per line
#define DELIMITERS " \t\n\v\f\r"
std::string init_msg("Welcome to MyShell v1.0.\n\nIt is a overlay over the Linux Terminal and is designed to solve the problem presented in\nProject 1 of Chapter 3\nOperating System Concepts Book\n\nIt is an C++ implementation of the original works of \n\"forestLoop\", (C language)\nRepository: https://github.com/forestLoop.\n\nCreated By: Harshal Dhake, 191EE212 for CS252 Assignment in the academic year 2021.\n");

//Functions for Handling the commands.

//------------> Initialization Functions

/*
1. Function: init_args
------------------------------------
    Initializing args array

    Parameters: char* args[] -> Array to be initialized
*/
void init_args(std::vector<std::string> &args) {
    args.clear();
    args.assign(MAXLINE/2 + 1, "");

    return;
}

/*
2. Function: init_command
------------------------------------
    Initializing command string

    Parameters: char* command -> String to be initialized to Empty
*/
void init_command(std::string &command) {
    command.erase();

    return;
}

//------------> I/O Functions
/*
Function: std_scan
------------------------------------
    Get input from the terminal.
*/
bool std_scan(std::string &command) {
    std::string inputBuffer;
    std::stringstream inputStream;

    getline(std::cin, inputBuffer);

    if(inputBuffer.length() == 0) {
        printf("No Command entered! Please enter a command or \"exit\" to terminate.\n");
        return false;
    }
    if(inputBuffer == "!!") {
        if(command.length() == 0) {
            printf("No command history.\n");
            return false;
        } else {
            printf("%s\n", command.c_str());
            return true;
        }
    }

    command = inputBuffer;
    return true;
}

/*
Function: parse_args_from_cmd
------------------------------------
    Parse the input received from the command line and find number of args,
    store it in the args array.
*/
size_t parse_args_from_cmd(std::vector<std::string>& args, std::string command) {
    char copy_command[command.length() + 1];
    size_t num = 0;
    strcpy(copy_command, command.c_str());

    char* token_ptr = strtok(copy_command, DELIMITERS);
    while(token_ptr != NULL) {
        args[num] = std::string(token_ptr);
        num++;
        token_ptr = strtok(NULL, DELIMITERS);
    }
    args[num] = '\0';

    return num;
}

//------------> Backend IMP Functions
/*
Function: check_io_redirect
------------------------------------
    Checks if any redirection of input/output stream is requested in the command,
    and filter out those tokens from the command.

    Note: Maximum of 4 redirection tokens can be present in the command,
    two for filenames for each input and output.

    Returns bool IO flag (0th Bit-> Input Bit, 1st Bit-> Output Mode)
*/
unsigned check_io_redirect(std::vector<std::string>& args, size_t& size, std::string& input_file, std::string& output_file) {
    unsigned flag = 0;
    size_t to_remove[4] = {0}, remove_cnt = 0;
    for(size_t i = 0; i != size; i++) {
        if(remove_cnt >= 4) {
            break;
        }
        if(args[i] == "<") {
            to_remove[remove_cnt++] = i;
            if(i == size-1) {
                fprintf(stderr, "No Input File has been provided to redirect Input Stream.\n");
                break;
            }
            flag |= 1; //Setting the Input Redirection Bit ON.
            input_file = args[i+1];
            to_remove[remove_cnt++] = ++i; //Removing the file name also from the main command.
        } else if(args[i] == ">") {
            to_remove[remove_cnt++] = i;
            if(i == size-1) {
                fprintf(stderr, "No Output File has been provided to redirect Output Stream.\n");
                break;
            }
            flag |= 2; //Setting the Output Redirection Bit ON.
            output_file = args[i+1];
            to_remove[remove_cnt++] = ++i; //Removing the file name also from the main command.
        }
    }
    //Removing the tokens now from the main commmand after detecting them and also the filenames
    //Using int here in stead of size_t as size_t can't be used in decrementing loops, as it enters infinite loops.
    for(int i = remove_cnt-1; i>=0; --i) {
        size_t pos = to_remove[i];
        while(args[pos].length() != 0) {
            args[pos] = args[pos+1];
            ++pos;
        }
        size--;
    }

    return flag;
}

/*
Function: redirect_io
------------------------------------
    Takes in the IO Flag (2bit Unsigned), input and output files and the descriptors[kernel-indices] for
    the files and redirects the stream to the file

    return bool success or failure.(T/F resp.)
*/
bool open_file_redirect_io(unsigned io_flag, std::string& input_file, std::string& output_file, int& input_desc, int& output_desc) {
    if(io_flag & 2) {
        //Redirecting the Output Stream
        output_desc = open(output_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 644);
        if(output_desc < 0) {
            fprintf(stderr, "Failed to open the output file,\n%s\n", output_file.c_str());
            return false;
        }
        dup2(output_desc, STDOUT_FILENO);
    }
    if(io_flag & 1) {
        //Redirecting the Input Stream
        input_desc = open(input_file.c_str(), O_RDONLY, 644);
        if(input_desc < 0) {
            fprintf(stderr, "Failed to open the input file,\n%s\n", input_file.c_str());
            return false;
        }
        dup2(input_desc, STDIN_FILENO);
    }

    return true;
}

/*
Function: close_io_files
------------------------------------
    Closes the opened IO Files.
*/
void close_io_files(unsigned io_flag, int input_desc, int output_desc) {
    if(io_flag & 2) {
        close(output_desc);
    }
    if(io_flag & 1) {
        close(input_desc);
    }

    return;
}

/*
Function: check_concurrent_processes
------------------------------------
    Checks for the '&' token (in the end of args), removes it if present.

    return true if present.
*/
bool check_concurrent_processes(std::vector<std::string>& args, size_t& size) {
    //if & not at the end
    size_t len = args[size-1].length();
    if(args[size-1][len-1] != '&') {
        return false;
    }
    if(len == 1) {
        //Remove if last arg is just '&'
        args[size-1].erase();
        size--;
    } else {
        //if '&' is along with the last arg e.g. abcd&, just remove the &
        args[size-1].pop_back();
    }

    return true;
}

/*
Function: detect_pipe
------------------------------------
    Detect the Pipe '|' toekn and split the args accordingly.
*/
void detect_pipe(std::vector<std::string>& args, size_t& num_of_args, std::vector<std::string>& args2, size_t& num_of_args2) {
    for(size_t i = 0; i != num_of_args; i++) {
        if(args[i] == "|") {
            args[i].erase();
            num_of_args2 = num_of_args - i - 1;
            for(size_t j = i+1; j != num_of_args; j++) {
                args2[j-i-1] = args[j];
                args[j].erase();
            }
            num_of_args = i;
            break;
        }
    }
}


/*
Function: run_cmd
------------------------------------
    Runs the Command obtained using the args after parsing the input command.
    returns bool, success or failure.
*/
bool run_cmd(std::vector<std::string>& args, size_t num_of_args) {
    bool run_concurrently = check_concurrent_processes(args, num_of_args);

    //Detecting Pipe
    std::vector<std::string> args2;
    size_t num_of_args2 = 0;
    detect_pipe(args, num_of_args, args2, num_of_args2);

    //Now to execute the requested command we create a child process (in Linux)
    //as the parent process is the shell! (in Linux)
    pid_t pid = fork();
    if(pid < 0) {
        fprintf(stderr, "Failed to fork and execute the command.\n");
        return false;
    } else if (pid == 0) {
        //Child Process
        if(num_of_args2 != 0) {
            //Pipe Exists
            int file_desc[2];   //file_desc[0] -> Read Head, file_desc[1] -> Write Head of the Pipe
            pipe(file_desc);

            //Fork Another two processes for communication
            pid_t pid2 = fork();
            if(pid2 > 0) {
                //Child Process of the 2nd Command

                //Checking for IO Redirection
                std::string input_file, output_file;
                int input_desc, output_desc;
                unsigned io_flag = check_io_redirect(args, num_of_args, input_file, output_file);
                
                io_flag &= 2; //Disabling the Input Redirection as it'll be taking data from 1st Command.
                
                if(open_file_redirect_io(io_flag, input_file, output_file, input_desc, output_desc) == false) {
                    //Either of the redirection failed
                    return false;
                }
                
                close(file_desc[1]);    //Closing the Write Head
                dup2(file_desc[0], STDIN_FILENO);   //Setting the Read Head as the Input for the 2nd Command.

                wait(NULL);     //Waiting for 1st Command to finish and write data to Write Head, to be buffered in the Read Head.

                //Creating char **args2_c for execvp
                char *args2_c[MAXLINE/2 + 1];
                for(size_t i = 0; i != num_of_args2; i++) {
                    args2_c[i] = (char*) malloc(args2[i].length() + 1);
                    strcpy(args2_c[i], args2[i].c_str());
                }

                execvp(args2[0].c_str(), args2_c);    //Execute the 2nd Command and whatever input it requires from STDIN comes from the Read Head.
                
                //Freeing the duplicated args
                for(size_t i = 0; i != num_of_args2; i++) {
                    free(args2_c[i]);
                }

                close_io_files(io_flag, input_desc, output_desc);
                close(file_desc[0]);

                fflush(stdin);
            } else if(pid2 == 0) {
                //Actual Process which will handle the 1st Command when a pipe is present (hence like a grandchild)

                //Checking for IO Redirection
                std::string input_file, output_file;
                int input_desc, output_desc;
                unsigned io_flag = check_io_redirect(args, num_of_args, input_file, output_file);

                io_flag &= 1;   //Disabling the Output Redirection as it'll be sending data to 2nd Command.

                if(open_file_redirect_io(io_flag, input_file, output_file, input_desc, output_desc) == false) {
                    //Either of the redirection failed
                    return false;
                }

                close(file_desc[0]);    //Closing the Read Head
                dup2(file_desc[1], STDOUT_FILENO);      //Setting the Write Head as the Output for the 1st Command.                

                //Creating char **args_c for execvp
                char *args_c[MAXLINE/2 + 1];
                for(size_t i = 0; i != num_of_args; i++) {
                    args_c[i] = (char*) malloc(args[i].length() + 1);
                    strcpy(args_c[i], args[i].c_str());
                }

                execvp(args[0].c_str(), args_c);      //Execute the 1st Command and whatever output it produces moves from STDOUT to the Write Head. 

                //Freeing the duplicated args
                for(size_t i = 0; i != num_of_args; i++) {
                    free(args_c[i]);
                }

                close_io_files(io_flag, input_desc, output_desc);
                close(file_desc[1]);

                fflush(stdin);
            }
        } else {
            //No Pipe
            //Checking IO Redirection
            std::string input_file, output_file;
            int input_desc, output_desc;
            unsigned io_flag = check_io_redirect(args, num_of_args, input_file, output_file);
            if(open_file_redirect_io(io_flag, input_file, output_file, input_desc, output_desc) == false) {
                //Either of the redirection failed
                return false;
            }

            //Creating char **args_c for execvp
            char *args_c[MAXLINE/2 + 1];
            for(size_t i = 0; i != num_of_args; i++) {
                args_c[i] = (char*) malloc(args[i].length() + 1);
                strcpy(args_c[i], args[i].c_str());
            }

            execvp(*args_c, args_c);
            
            //Freeing the duplicated args
            for(size_t i = 0; i != num_of_args; i++) {
                free(args_c[i]);
            }
            
            close_io_files(io_flag, input_desc, output_desc);
            fflush(stdin);
        }
    } else {
        //Parent Process
        if(!run_concurrently) {
            wait(NULL);
        }
    }

    return true;
}



/*
The Main Function of MyShell.

Handles: 
1. Contiuous Looping till exit entered.
2. Function Calls as per commands entered
3. Refereshing / Flushing the System before starting, to avoid memory leaks.
*/
int main(void) {
    std::vector<std::string> args; // Command Line of 80 Chars, can have max 40 arguments
    std::vector<std::string> args2; // Command Line of 80 Chars, can have max 40 arguments
    std::string command; 

    init_args(args);
    init_command(command);

    printf("%s", init_msg.c_str());

    //Main Loop of the Shell
    while(1) {
        printf("myshell_v1.0> ");
        fflush(stdin);
        fflush(stdout);

        init_args(args);

        if(!std_scan(command)) continue;

        size_t num_of_args = parse_args_from_cmd(args, command);

        if(args[0] == "exit") break;
        if(num_of_args == 0) {
            printf("No valid command entered. Please enter a valid command.\nOr enter \"exit\", without quotes to terminate the session.");
            continue;
        }

        run_cmd(args, num_of_args);
    }

    args.clear();
    return 0;
}