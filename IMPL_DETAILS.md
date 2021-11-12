# Implementation Details for MyShell v1.0

## I. Overview
- Implementing MyShell, fundamentally has three parts:
    - Read input from the User,
    - Parse the input,
    - Execute the requested command. 

- For I/O redirection, Read and Writing Files have to be bound to  `stdin` and `stdout`. 
- MyShell v1.0 (currently) supports only a single pipe, rather than multiple chained pipes. The pipe is implemented using `fork()` and communication between them with `pipe()`.

## II. Control Flow of Operations
The Flow of operations in MyShell can be described as:

- **Initialization**: Initializing the ```args``` and ```command``` to empty states to avoid memory leaks and garbage values entering the program during its initial execution.

- **Input Reading & Parsing**: Continuously getting the input from user and parsing it to obtain the command to be executed along with its arguments and any other secondary operations like I/O Redirection, Concurrent Processing or Usage of Pipes.

- **Running the Command**: Running the command is more involved and intricate process as compared to the steps covered so far. It can be divided into stages,

    - Obtaining if Pipe and/or concurrent behaviour is requested.
    - If Pipe is requested, divide the command into two parts for two separate commands and argument lists.
    - Fork the current process to execute the command, as the current process is infact the MyShell program running.
    - On forking the process, the child process can be detected by ```pid = 0``` in the ```if``` statement. Now this handles the actual command requested. It branches into two parts:
        
        - If the *original command* required pipe then once more we ```fork``` the process and handle the two parts of command. The parent process handles the first command which is at the Read End of the Pipe and the child process handles the second part of the command which is at the Write End of the Pipe.

        - If the *original command* didn't require pipe then directly handle the command in the current child process.

    - Handling the process implies that:

        - Checking I/O Redirection for the command.
        - For "pipe" based requests, we close the opposite end of the pipe (to avoid accidental read/writes) and using ```dup2``` associate ```STDIN_FILENO``` and ```STDOUT_FILENO``` (0 and 1 resp., defined in *unistd.h* ) to Read Input and Write Output so that the other end of the pipe can access it.
        - Generate ```char **``` formatted arguments for ```execvp```.
        - Execute the command using ```execvp```
        - Free the duplicated arguments, to avoid any memory leaks.

    - If ```pid > 0``` meaning parent process is executing then we check if concurrent behaviour is required and wait accordingly.

## III. Linux Commands
The following commands have been used and my brief understanding of each of these commands have been given along with the problems I have faced whilst using those commands:

- **fork()** <br/>
    Fork Command defined in ***unistd.h***, creates a new process by duplicating the calling process. The Calling process is the ***Parent Process***, whilst the duplicated process is ***Child Process***. 

    The child process and the parent process run in separate memory
    spaces.  At the time of fork() both memory spaces have the same
    content. 
    
    The child process is an exact duplicate of the parent process
    except for:

    - The child has its own unique process ID, and this PID does not match the ID of any existing process group.
    - The child's parent process ID is the same as the parent's process ID.
    - The child does not inherit semaphore adjustments from its parent. 
    - The child process is created with a single thread—the one that called fork(). The entire virtual address space of the parent is replicated in the child including the states of mutexes.

    ### Return Value: 
    On success, the PID of the child process is returned in the parent, and **0** is returned in the child.  On failure, -1 is returned in the parent, no child process is created, and ***errno*** is set to indicate the error.

- **pipe(pipefd[2])** <br/>
    Pipe Command defined in ***unistd.h***, creates a pipe, a unidirectional data channel that can be used for interprocess communication.  The array pipefd is used to return two file descriptors referring to the ends of the pipe. pipefd[0] refers to the read end of the pipe.  pipefd[1] refers to the write end of the pipe.  Data written to the write end of the pipe is buffered by the kernel until it is read from the read end of the pipe.

    **pipe()** does not modify pipefd on failure, only on success it modifies the passed array giving two file descriptors.

- **dup2(oldfd, newfd)** <br/>
    The dup2() system call allocates a new file descriptor that refers to the same open file description as the descriptor **oldfd**. It uses the file descriptor number specified in **newfd**.  In other words, the file descriptor newfd is adjusted so that it now refers to the same open file description as oldfd. If the file descriptor newfd was previously open, it is closed before being reused; the close is performed silently (i.e., any errors during the close are not reported by dup2()).

    *Note: dup() uses the lowest numbered file descriptor unused in the calling process.*

- **close(fd)** <br/>
    Close Command defined in ***unistd.h***, closes a file descriptor, so that it no longer refers to any file and may be reused.  Any record locks held on the file it was associated with, and owned by the process, are removed (regardless of the file descriptor that was used to obtain the lock).

    *Note: Record Locks are the O_ used during file handling, defined in **fcntl.h***

- **wait(NULL)** <br/>
    The wait system call is defined in ***sys/wait.h***. The wait() system call suspends execution of the calling thread until one of its children terminates.

- **execvp(argc, argv)** <br/>
    The Execvp Command defined in ***unistd.h***, executes the file provided by its location in first parameter *argc* as **(char * )** and its arguments in the second parameter *argv* as **(char ** )**.

    ### Problems Faced:
    - Cannot use ```c_str()``` method of the ```std::string``` class to pass paramters to the ```execvp``` command as ```c_str()``` returns ```const char *``` which isn't accepted. Hence needed to manually use ```(char *) malloc``` to duplicate the arguments so that they can be passed.

    - The list of arguments *argv* must be terminated by **NULL**, which I hadn't ensured while converting ```std::vector<std::string> args```  to ```char **```, causing only single argument commands to run like ```ls``` or ```clear``` etc.

    ### Why choose ```execvp``` amongst others in ```exec``` family?
    - ```execvp``` inputs file in the first parameter as opposed to pathnames in the standard ```execl```, ```execv``` and argumented ```execle```.
    - ```execvp``` inputs an array of arguments in the format ```char **``` as opposed to a single argument (```const char *```) in ```execlp```.
    - Also no enviroment variables were required for the implementation hence ```execvpe``` was not used.

