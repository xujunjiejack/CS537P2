#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>


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

    builtin_cmd built_cmd_type;
    // cmd
    // file descriptor with its name
    // background
    // argument
    // builtin_cmd

} Command;

Command* parse_command(char* cmd);

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

    return cmd;
}

int cmd_is_builtin(Command* cmd);
int perform_builtin_function(Command* cmd);

int main() {
    printf("Hello, World!\n");

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
    char str[] = "pwd";
    Command* cmd = parse_command(str);

    print_cmd_status(cmd);
    // if built-in, no fork. search in the built in list to make sure the cmd in built-ins
    // support
    if (cmd_is_builtin(cmd)){

        // This function returns 0 to indicate not success.
        if (perform_builtin_function(cmd)){
            // jump out
        } else {
            // let's see whether this brunch is necessary
            // jump out
        };
    }

    // if user program, fork process
    // fork

    // check the file descriptor
    // Given a specific file descriptor, such as "<" or ">", the corresponding
    // stdin or stdout should be set to read the data from the file
    // For stdout, the open() with the flags O-WRONLY, O_CREATE, and
    // O_TRUNC can be useful
    // For stdin, the open() should be executed with the O_RDONLY flag.

    // check &. If a command is issued with this character, this command is seen as
    // a background process. The background process doesn't require the shell
    // to wait for it to proceed. Just leave the background process running itself
    // However, it's necessary to constant check whether this background process
    // has been dead. If yes, then reap it.

    // then for child, run
    // (with execvp)

    // for parent (shell), wait until the child exits.

    // free cmd;
    cmd = free_command(cmd);
    free(cmd);
    // Loop back
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

    // assign the file descriptor
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
                fprintf(stdout, "%s", cwd);
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

// run different process for regular process and background process
void fork_for_regular_process(){

}

void fork_for_background_process(){

}

// built in cd, pwd, exit
// cd with chdir(). Needs the argument from getenv("HOME")

// pwd. Use getcwd()

// exit (call exist);