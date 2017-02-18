///////////////////////////////////////////////////////////////////////////////
//                   ALL STUDENTS COMPLETE THESE SECTIONS
// Title:            sqysh
// Files:            sqysh.c
// Semester:         CS 537 Spring 537
//
// Author:           Junjie Xu
// Email:            jxu259@wisc.edu
// CS Login:         junjie
// Lecturer's Name:  Zev Weiss
//
//////////////////// PAIR PROGRAMMERS COMPLETE THIS SECTION ////////////////////

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

typedef enum{
    none = -1,
    _cd = 1,
    _pwd = 2,
    _exits = 3

}builtin_cmd;


typedef struct {

    char* cmd;                      // Command name
    char** argv;                    // Argument for that command, excluding file descriptor
    int argc;                       // Number of the argument
    int stdin_is_from_file;         // Decide whether we need to redirect the stdin from a file
    char* stdin_file_name;          // The source file name for stdin
    int stdout_is_from_file;        // Decide whether we need to redirect the stdoun to a file
    char* stdout_file_name;         // The output file for stdout
    int is_back_ground_process;     // Indicating whether is command is background process
    int pid;                        // Pid for this Command
    int is_running_background;      // Whether this command is currently running in background
    builtin_cmd built_cmd_type;     // the command type

} Command;

Command* parse_command(char* cmd);
int fork_for_background_process(Command* cmd);
void fork_for_regular_process(Command* cmd);
int cmd_is_builtin(Command* cmd);
int perform_builtin_function(Command* cmd);
int setup_stdin(Command* cmd);
int setup_stdout(Command* cmd);
void check_child_exits(Command** cmds, int count);
FILE* prepare_input_environment(int argc, char** argv, int* is_from_file);

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

/**
 * Given a command, this function will free the data inside,
 * and also this pointer
 * @param cmd
 * @return
 */
Command* free_command(Command * cmd){
    free(cmd->cmd);
    free(cmd->stdin_file_name);
    free(cmd->stdout_file_name);

    int i;
    for (i=0; i<cmd->argc; i++){
        free(cmd->argv[i]);
    }
    free(cmd->argv);
    free(cmd);
    return cmd;
}

/**
 * Given a read-in string. Get rid of "\n" at the end of the string
 *
 * @param str
 */
void clean_string(char* str){
    int i = 0;
    for (i = 0; i< strlen(str); i++){
        if (str[i] == '\n'){
            str[i] = '\0';
        }
    }
}

/**
 * Read in the data from the given stream. If the input source is stdin with the terminal,
 * I regard it as interactive mode
 *
 * @param argc number of the argument
 * @param command_str pass the command string out
 * @param stream stdin or file
 * @return
 */
int get_cmd_from_file(int argc, char* command_str, FILE *stream){

    if (argc == 1 && isatty(fileno(stdin))){
        printf("sqysh$ ");
    }


    if (fgets(command_str, 1024, stream) == NULL){
        return -2;
    };

    if (command_str[0] == '\n'){
        return -1;
    }

    clean_string(command_str);
    return 1;
}


int main(int argc, char ** argv) {

    Command** background_cmds = calloc(1, sizeof(Command));
    int background_count = 0;
    int is_from_file = 0;
    FILE* input_stream = prepare_input_environment(argc, argv, &is_from_file);

    while(1) {
        char command_str[1024];

        // process before data entrance
        check_child_exits(background_cmds, background_count);

        int read_in_status = get_cmd_from_file(argc, command_str, input_stream);

        if (read_in_status == -1){
            continue;
        }else if (read_in_status == -2){
            break;
        };

        check_child_exits(background_cmds, background_count);

        Command *cmd = parse_command(command_str);

       // print_cmd_status(cmd);

        // run builtin program
        if (cmd_is_builtin(cmd)) {
            // If this function returns -1, exit the program.
            if (perform_builtin_function(cmd) == -1) {
                free_command(cmd);
                break;
            }
            free_command(cmd);
            continue;
        }

        // run other process
        if (cmd->is_back_ground_process) {
            fork_for_background_process(cmd);

            // dynamically add cmd to cmd list
            // background process will be freed when shell exits
            background_count++;
            background_cmds = realloc(background_cmds,
                                          background_count * sizeof(Command *));
            background_cmds[background_count - 1] = cmd;
        } else {
            fork_for_regular_process(cmd);
            free_command(cmd);
        }
    }

    // Free the background lists
    int cmd_num = 0;
    for (cmd_num = 0; cmd_num< background_count; cmd_num++){
        free_command(background_cmds[cmd_num]);
    }
    free(background_cmds);

    if (is_from_file == 1){
        fclose(input_stream);
    }
    return 0;
}

/**
 * Given the argument, decide which stdin to use
 *
 * @param argc
 * @param argv
 * @param is_from_file, a pointer that passes whether the stdin is from a file
 * @return
 */
FILE* prepare_input_environment(int argc, char** argv, int* is_from_file){

    if (argc == 1){
        return stdin;
    }

    FILE* f;
    if (argc == 2){

        f = fopen(argv[1],"r");
        if (f == NULL){
            fprintf(stderr, "File not found");
            exit(1);
        }
        *is_from_file = 1;
        return f;
    }

    fprintf(stderr, "Sqysh currently doesn't support arguments more than 2");
    exit(1);
    return NULL;

    // check whether it's in environment mode or not
    // print out the sqysh$
}

/**
 * Wrapper for comparing whether the string is the same
 *
 * @param str
 * @param character
 * @return
 */
int string_equal(char *str, char *character){
    if (!strcmp(str, character)){
        return 1;
    }
    return 0;
}

/**
 * Given a string of command. Parse it to the data structure of Command
 * @param cmd
 * @return
 */
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
    // add null to the end of the args
    command->argv = realloc(command->argv, (command->argc+1) * sizeof(char*));
    command->argv[command->argc] = NULL;

    // don't forget to free the Command at last

    return command;
}

/**
 * Check whether a command is a builtin function
 * @param cmd Command
 * @return 1 if this command is a builtin function
 *         0 if this command is not
 */
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

/**
 * Redirect the stdin from the command
 * @param cmd
 * @return -1 for failure
 */
int setup_stdin(Command* cmd){
    if (cmd->stdin_is_from_file){

        int fd = 0;
        if ((fd = open(cmd->stdin_file_name, O_RDONLY, 0666)) == -1){
            fprintf(stderr, "Error on file redirecting");
            return -1;
        };

        if (dup2(fd, STDIN_FILENO) == -1){
            fprintf(stderr, "Error on file redirecting\n");
            return -1;
        };
        close(fd);
        return fd;
    }
    return -1;
}

/**
 * Redirect the stdout from the command
 * @param cmd
 * @return -1 if failure
 */
int setup_stdout(Command* cmd){
    if (cmd->stdout_is_from_file){
        int fd = 0;
        if ((fd = open(cmd->stdout_file_name, O_WRONLY | O_CREAT | O_TRUNC,
                       0666)) == -1 ){
            fprintf(stderr, "Error on file redirecting");
            return -1;
        };
        if (dup2(fd, STDOUT_FILENO) == -1){
            fprintf(stderr, "Error on file redirecting");
            return -1;
        };
        close(fd);
        return fd;
    }
    return -1;
}

/**
 * Execute a builtin command,
 * @param cmd user command
 * @return -1 if the shell needs to exit
 */
int perform_builtin_function(Command* cmd){
    char cwd[257];

    switch (cmd->built_cmd_type){
        case _cd :

            // if no argument
            if (cmd->argc == 1){
                if (chdir(getenv("HOME"))){
                    fprintf(stderr, "%s: %s\n", cmd->cmd, strerror(errno));
                    return 0;
                }else{
                    return 1;
                }
            }

            if (cmd->argc > 2){
                fprintf(stderr, "cd: too many arguments\n");
                return 1;
            }

            //Upon successful completion, 0 shall be returned.
            // This is not success case
            if (chdir(cmd->argv[1])){
                fprintf(stderr, "cd: %s: %s\n", cmd->argv[1], strerror(errno));
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
            return -1;
            break;

        default:
            fprintf(stderr, "This case should not be entered");
            return 0;
    }
}

/**
 * close a file descriptor if the file descriptor is not 1
 * @param fd
 */
void close_file(int fd){
    if (fd == -1){
        return;
    }else{
        close(fd);
    }
}

/**
 * fork for regular process and background process
 * @param cmd
 */
void fork_for_regular_process(Command* cmd){
    pid_t pid;
    int status;
    if ((pid = fork()) < 0){
        fprintf(stderr, "Forking child process failed \n");
        return;
    } else if (pid == 0){
        int fd_in = setup_stdin(cmd);
        int fd_out = setup_stdout(cmd);
        if (execvp(cmd->cmd, cmd->argv) < 0) {
            fprintf(stderr, "%s: %s\n", cmd->cmd, strerror(errno));
            close_file(fd_in);
            close_file(fd_out);
            exit(1);
        }

    } else{
        while (wait(&status) != pid){}
    }
}

// Return pid for this background process
/**
 * fork a backgound proces with redirecting
 * @param cmd the command
 * @return pid for the new child process
 */
int fork_for_background_process(Command* cmd){
    pid_t  pid;
    int status;
    if ((pid=fork()) < 0){

    }  else if (pid == 0){
        int fd_in = setup_stdin(cmd);
        int fd_out = setup_stdout(cmd);

        if (execvp(cmd->cmd, cmd->argv) < 0) {
            fprintf(stderr, "%s: %s\n", cmd->cmd, strerror(errno));
            cmd->is_running_background = 0;
        }

        close_file(fd_in);
        close_file(fd_out);
        exit(1);

    } else{
        cmd->pid = pid;
        cmd->is_running_background = 1;
        if (waitpid(-1, &status, WNOHANG) == pid){
            fprintf(stderr, "[%s (%d) completed with status %d]\n", cmd->cmd,
                    pid, status);
        }
    }
    return pid;
}

/**
 * Use waitpid to check whether there exists a child process exits, and print
 * out the information
 * given the background commands lists
 * @param background_cmds background lists
 * @param bg_cmds_count the count of the background
 */
void check_child_exits(Command** background_cmds, int bg_cmds_count){
    //
    int pid;
    int status;

    if (bg_cmds_count == 0){
        return;
    }

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0){
        for (int i=0; i<bg_cmds_count; i++){
            Command* cmd = background_cmds[i];
            if (cmd->pid == pid && cmd->is_running_background){
                fprintf(stderr, "[%s (%d) completed with status %d]\n",
                        cmd->cmd, pid, WEXITSTATUS(status));
                cmd->is_running_background = 0;
            }
        }
    }
    return;
}