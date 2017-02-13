#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/wait.h>

// No mechanism for escaping or quoting whitespace (or anything else) is needed

//Input lines will be at most 256 characters (including the trailing '\n')

typedef enum{
    none = -1,
    _cd = 1,
    _pwd = 2,
    _exits = 3

}builtin_cmd;

typedef struct {

    char* cmd;

    char** argv;

    int argc;

    int stdin_is_from_file;

    char* stdin_file_name;

    int stdout_is_from_file;

    char* stdout_file_name;

    int is_back_ground_process;

    int pid;

    int is_running_background;

    builtin_cmd built_cmd_type;
    // cmd
    // file descriptor with its name
    // background
    // argument
    // builtin_cmd

} Command;

Command* parse_command(char* cmd);
void on_exiting(Command* cmd);
int fork_for_background_process(Command* cmd);
void fork_for_regular_process(Command* cmd);
int cmd_is_builtin(Command* cmd);
int perform_builtin_function(Command* cmd);
void setup_stdin(Command* cmd);
void setup_stdout(Command* cmd);
void check_child_exits(Command** cmds, int count);
int test_number = 1;

void print_cmd_status(Command* cmd){
    printf("The cmd name: %s\n", cmd->cmd);

    printf("The argument number: %d\n", cmd ->argc);
    printf("The argument is ");

    int i;
    for (i = 0; i<cmd->argc; i++){
        printf("%s ", cmd->argv[i]);
    }
    printf("\n");

    printf("stdin file flag: %d\n", cmd->stdin_is_from_file);
    printf("stdin file : %s \n", cmd->stdin_file_name);

    printf("stdout file flag: %d\n", cmd->stdout_is_from_file);
    printf("stdout file : %s \n", cmd->stdout_file_name);
    printf("Is background: %d \n", cmd->is_back_ground_process);
}

Command* free_command(Command * cmd){
    free(cmd->cmd);
    free(cmd->stdin_file_name);
    free(cmd->stdout_file_name);

    int i;
    for (i=0; i<cmd->argc; i++){
        free(cmd->argv[i]);
    }
    free(cmd->argv);

    return cmd;
}


int main() {

    // printf("Hello, World!\n");
    Command** background_cmds = calloc(1, sizeof(Command));
    int background_count = 0;
    // How to write a shell
    // Before input, try to determine whether the shell should enter
    // interactive mode
    // If yes, print out the string
    // Use isatty() to check. Man isatty()


    // If not, not print out the string. That's the only difference

    // Read in input

    // read in cmd

    // parse cmd (built-in or user program, whether has file descriptor)
    // How can I parse. In documentation, it says parsing by whitespace
    // Then use the first string as the command
    //char str[] = "1 2 3 < test.txt > hello ";
    int count = 0;
    while(1) {

        check_child_exits(background_cmds, background_count);
        char command_str[1024];
        fgets(command_str, 1024, stdin);
        check_child_exits(background_cmds, background_count);

        printf("---------\nrunning time: %d\n for command %s\n", count, command_str);
        count++;
        char str[] = "hello > hello.txt &";
        Command *cmd = parse_command(command_str);

        print_cmd_status(cmd);
        // if built-in, no fork. search in the built in list to make sure the cmd in built-ins
        // support
        if (cmd_is_builtin(cmd)) {

            // This function returns 0 to indicate not success.
            // If this function returns -1, exit the program.

            if (perform_builtin_function(cmd) == -1) {
                break;
            }
            continue;
        }


        // check the file descriptor
        // Given a specific file descriptor, such as "<" or ">", the corresponding
        // stdin or stdout should be set to read the data from the file
        // For stdout, the open() with the flags O-WRONLY, O_CREATE, and
        // O_TRUNC can be useful
        // For stdin, the open() should be executed with the O_RDONLY flag.\
    // Do I need to close the file before leaving?
        // if user program, fork process
        // fork

        if (cmd->is_back_ground_process) {
            fork_for_background_process(cmd);
                // dynamically add cmd to cmd list
            background_count++;
            background_cmds = realloc(background_cmds,
                                          background_count * sizeof(Command *));
            background_cmds[background_count - 1] = cmd;
            // background process will be freed when shell ends
        } else {
            fork_for_regular_process(cmd);
            free_command(cmd);
        }

        // check &. If a command is issued with this character, this command is seen as
        // a background process. The background process doesn't require the shell
        // to wait for it to proceed. Just leave the background process running itself
        // However, it's necessary to constant check whether this background process
        // has been dead. If yes, then reap it.

        // then for child, run
        // (with execvp)

        // for parent (shell), wait until the child exits.
    }

    printf("Shell exits");
    // TODO: free cmd and cmd lists;
    // on_exiting(cmd);
    // Loop back

    int cmd_num = 0;
    for (cmd_num = 0; cmd_num< background_count; cmd_num++){
        free_command(background_cmds[cmd_num]);
    }
    free(background_cmds);
    return 0;
}


void prepare_input_environment(){
    // check whether it's in environment mode or not
    // print out the sqysh$
}

int string_equal(char *str, char *character){
    if (!strcmp(str, character)){
        return 1;
    }
    return 0;
}

// I'm thinking of providing a data structure to save the command issue
Command* parse_command(char* cmd){

    // the parser actually is a iterator
    // parse the string into array
    // iterate through the string to find file descriptor
    // initialize the Command

    Command * command = calloc(1,sizeof(Command));

    char *arg = "";
    command->argc = 0;
    command->argv = calloc(1, sizeof(char*));
    char token[] = " ";
    arg = strtok(cmd, token);

    char ** fd_name = NULL;

    while(arg != NULL){

        // If it's a file descriptor, don't save it in the argv and don't update the argc
        // How to read the next one
        // Or I can set a fd name reader. Put the arg there if there is some thing

        if(string_equal(arg, "<")){
            command->stdin_is_from_file = 1;
            fd_name = &(command->stdin_file_name);
            arg = strtok(NULL, " ");
            continue;
        }

        // find the file descriptor
        if (string_equal(arg, ">")){

            command->stdout_is_from_file = 1;
            fd_name = &(command->stdout_file_name);
            arg = strtok(NULL, " ");
            continue;
        }

        if (string_equal(arg, "&")){
            command->is_back_ground_process = 1;
            break;
        }

        // redirect the argument to the place in Command
        if (fd_name != NULL){
            *fd_name = malloc((strlen(arg)+1) * sizeof(char));
            strcpy(*fd_name, arg);
            fd_name = NULL;
            arg = strtok(NULL, " ");
            continue;
        }

        // Update the argc and argv. Deal with the regular file
        command->argc ++;

        // the first string store as the cmd name
        if(command->argc == 1){
            command->cmd = malloc((strlen(arg) + 1) * sizeof(char));
            strcpy(command->cmd, arg);
        }

        // save the argument to the argments array by using realloc
        command->argv = realloc(command->argv,command->argc * sizeof(char*));
        command->argv[(command->argc)-1] = malloc((strlen(arg)+1)*sizeof(char));
        strcpy(command->argv[(command->argc) -1], arg);
        arg = strtok(NULL, " ");
    }

    // don't forget to free the Command at last

    return command;
}

int cmd_is_builtin(Command* cmd){
    if (string_equal(cmd->cmd,"cd")){
        cmd->built_cmd_type = _cd;
        return 1;
    }

    if(string_equal(cmd->cmd, "pwd")){
        cmd->built_cmd_type = _pwd;
        return 1;
    }

    if(string_equal(cmd->cmd, "exit")){
        cmd->built_cmd_type = _exits;
        return 1;
    }

    cmd->built_cmd_type = none;
    return 0;
}

void setup_stdin(Command* cmd){
    if (cmd->stdin_is_from_file){
        int fd = open(cmd->stdin_file_name , O_RDONLY, 0666);
        if (dup2(fd, STDIN_FILENO) <1){
            fprintf(stderr, "Error on file redirecting");
        };
    }
}

void setup_stdout(Command* cmd){
    if (cmd->stdout_is_from_file){
        int fd = open(cmd->stdout_file_name, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (dup2(fd, STDOUT_FILENO)<1){
            fprintf(stderr, "Error on file redirecting");
        };
    }
}

int perform_builtin_function(Command* cmd){
    char cwd[256];
    switch (cmd->built_cmd_type){
        case _cd :

            // if no argument gets
            if (cmd->argc == 1){
                if (chdir(getenv("HOME"))){
                    fprintf(stderr, "%s: %s\n", cmd->cmd, strerror(errno));
                    return 0;
                }else{
                    return 1;
                }
            }

            //Upon successful completion, 0 shall be returned.
            // This is not success case
            if (chdir(cmd->argv[1])){
                fprintf(stderr, "%s: %s\n", cmd->cmd, strerror(errno));
                return 0;
            }
            return 1;

        case _pwd:
            if (getcwd(cwd, sizeof(cwd)) != NULL){
                fprintf(stdout, "%s\n", cwd);
                return 1;
            }else {
                fprintf(stderr, "%s: %s\n", cmd->cmd, strerror(errno));
            }
            return 0;
            break;

        case _exits:
            exit(0);
            break;

        default:
            fprintf(stderr, "This case should not be entered");
            return 0;
    }
}

void on_exiting(Command* cmd){
    free_command(cmd);
    free(cmd);
}

// run different process for regular process and background process
void fork_for_regular_process(Command* cmd){
    pid_t pid;
    int status;
    if ((pid = fork()) < 0){
        printf(stderr, "Forking child process failed \n");
        exit(1);
    } else if (pid == 0){
        setup_stdin(cmd);
        setup_stdout(cmd);
        if (execvp(cmd->cmd, cmd->argv) < 0);
            printf(stderr, "exec failed\n");
    } else{
        while (wait(&status) != pid){

        }
    }
}

// Return pid for this background process
int fork_for_background_process(Command* cmd){
    pid_t  pid;
    int status;
    if ((pid=fork()) < 0){

    }  else if (pid == 0){
        setup_stdin(cmd);
        setup_stdout(cmd);

        if (execvp(cmd->cmd, cmd->argv) < 0) {
            printf(stderr, "exec failed\n");
            cmd->is_running_background = 0;
            // TODO: Add file close for file descriptor.
        }

    } else{
        cmd->pid = pid;
        cmd->is_running_background = 1;
        if (waitpid(-1, &status, WNOHANG) == pid){
            fprintf(stderr, "[%s (%d) completed with status %d]\n", cmd->cmd, pid, status);
        }
    }
    return pid;
}

void check_child_exits(Command** background_cmds, int bg_cmds_count){
    int pid;
    int status;

    if (bg_cmds_count == 0){
        return;
    }

    if ((pid = waitpid(-1, &status, WNOHANG)) > 0){
        for (int i=0; i<bg_cmds_count; i++){
            Command* cmd = background_cmds[i];
            if (cmd->pid == pid && cmd->is_running_background){
                fprintf(stderr, "[%s (%d) completed with status %d]\n",
                        cmd->cmd, pid, status);
                cmd->is_running_background = 0;
                return;
            }
        }
    }
}
// built in cd, pwd, exit
// cd with chdir(). Needs the argument from getenv("HOME")

// pwd. Use getcwd()

// exit (call exist);