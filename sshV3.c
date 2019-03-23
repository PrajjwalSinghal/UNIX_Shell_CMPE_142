#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
//******************//
pid_t wpid;
int status = 0;
int is_bad_batch_file = 0;
char* correct_path; // USED IN EXECVP CALL BCS PATH WASN'T WORKING OTHERWISE
//******************//
// ********* FUNCTIONS ***********//

//******* main_shell() is the starting point of the shell *************//
void main_shell(int is_interactive, char* filename, char** path_array, int paths_size);
//******* parse() breaks the input into separate commands *************//
int parse(char *command, char** arguments_array);
unsigned is_built_in_command(char* executable);
unsigned is_valid_system_command(char* executable, char** path_array, int paths_size);
void run_built_in_commands(char** arguments_array, int num_of_args, char** path_array, int *paths_size);
void run_system_commands(char** arguments_array, int num_of_args);
// is_redirected() checks if the output needs to go to a file or not ********//
int is_redirected(char** arguments_array, int num_of_args);
// ******************************//

int main(int argc, char* argv[])
{
    //******** STRING ARRAY TO STORE ALL PATH LOCATIONS ********************//
    char **path_array = calloc(10,sizeof(char*));
    path_array[0] = strdup("/bin/");
    path_array[1] = strdup("/usr/bin/");
    int paths_size = 2;     // SIZE OF THE ARRAY
    // *********************************************************************** //
    if(argc == 1){      // if no arguments are passed
        main_shell(1, "stdin", path_array, paths_size);
    }
    else if(argc == 2){ // if input file is passed
        main_shell(0, argv[1], path_array, paths_size);
    }
    else{
        char error_message[30] = "An error has occurred\n";
        write(STDERR_FILENO, error_message, strlen(error_message));
        return 1;
    }

    return 0;
}
int parse(char* command, char** arguments_array)
{
    char* found;
    char* temp1;
    char* temp2;
    int is_command_empty = 0;
    unsigned i = 0;
    while(1)
    {
        
        found = strsep(&command, " ");
        if(found == NULL)
            break;
        if(strcmp(found, "") == 0)
            continue;
        temp1 = strdup(found);
        temp2 = strsep(&temp1, ">");// If the redirection operator id not separated by " "
        if(temp1 == NULL || (strcmp(found, ">") == 0))
        {
            arguments_array[i] = strdup(found);
            i++;
        }
        else
        {
            arguments_array[i] = strdup(temp2);
            i++;
            arguments_array[i] = strdup(">");
            i++;
            arguments_array[i] = strdup(temp1);
            i++;
        }
    }
    
    return i;
}
unsigned is_built_in_command(char* executable)
{
    
    if(strcmp(executable, "exit") == 0)
    return 1;
    else if(strcmp(executable, "cd") == 0)
    return 1;
    else if(strcmp(executable, "path") == 0)
    return 1;
    
    return 0;
}
unsigned is_valid_system_command(char* executable, char** path_array, int paths_size)
{
    if(access(executable, X_OK) == 0)
    {
        return 1;
    }
    char *path;
    for(int i = 0; i < paths_size; i++)
    {
        path = calloc(50, sizeof(char));
        strcat(path, path_array[i]);
        strcat(path, executable);       // concatinate the paths with the command
        if(access(path, X_OK) == 0)
        {
            correct_path = strdup(path_array[i]);// correct_path stores the path where the exec file is
            free(path);
            return 1;
        }
        free(path); // FREE PATH BCS IT IS REALLOCATED
    }
    return 0;
}
void run_built_in_commands(char** arguments_array, int num_of_args, char** path_array, int *paths_size)
{
    if(strcmp(arguments_array[0], "exit") == 0){
        //********* 0 : shows errorless exit ****************************//
        if(num_of_args != 1){    //*** If more than 1 arguments are passes ***//
            char error_message[30] = "An error has occurred\n";
            write(STDERR_FILENO, error_message, strlen(error_message));
        }
        else
            exit(0);
    }
    else if(strcmp(arguments_array[0], "cd") == 0){
        if(num_of_args != 2){    //*** If more than 1 arguments are passes ***//
            char error_message[30] = "An error has occurred\n";
            write(STDERR_FILENO, error_message, strlen(error_message));
        }
        else {      //*** CHANGING THE DIRECRORY ***//
            chdir(arguments_array[1]);
        }
    }
    // PATH COMMAND ALWAYS OVERWRITES THE OLD PATHS
    else {
        *paths_size = num_of_args - 1;
        for(int i = 0; i < *paths_size; i++)
        {
            char* temp = calloc(100, sizeof(char));
            strcpy(temp, arguments_array[i+1]);
            strcat(temp, "/");
            path_array[i] = strdup(temp);
            free(temp);
        }
    }
}
void run_system_commands(char** arguments_array, int num_of_args)
{
    int rc = fork();
    if(rc < 0){
        printf("Fork Failed : ERROR\n");
        exit(1);
    }
    else if(rc == 0){
        //*** is_redirected RETURNS THE INDEX WHERE THE FILENAME IS STORED IN THE ARRAY ***//
        int file_index = is_redirected(arguments_array, num_of_args);
        //printf("File Index %d\n", file_index);
        if(file_index == -10)   // -10 indicates error
            return;
        if(file_index != -1){
            //*** CLOSING THE STANDARD OUTPUT AND OPENING THE FILE ***//
            close(STDOUT_FILENO);
            open(arguments_array[file_index], O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);
            //*** TO MAKE SURE THE "> FILENAME" DOES NOT INTERFERE WITH EXECUTION ***//
            arguments_array[file_index-1] = NULL;
        }
        //******* EXECVP THE COMMAND : ERROR IS HANDLED BY EXECVP ********//
        strcat(correct_path, arguments_array[0]);
        execvp(correct_path, arguments_array);
    }
    else{
        //******* PARENT WAITS FOR THE CHILD TO FINISH IN THE INTERACTIVE MODE *****************//
        return;
    }
}
int is_redirected(char** arguments_array, int num_of_args)
{
    
    for(unsigned i = 0; i < num_of_args; i++)
    {
        if(strcmp(arguments_array[i], ">") == 0)
        {
            if(num_of_args > i+2 || num_of_args == i+1)        // IF BAD ARGUMENTS ARE PASSED
            {
                char error_message[30] = "An error has occurred\n";
                write(STDERR_FILENO, error_message, strlen(error_message));
                return -10; // this indicates error
            }
            return (i+1);
        }
    }
    return -1;
}
void main_shell(int is_interactive, char* filename, char** path_array, int paths_size)
{
    FILE *input_file;
    if(!is_interactive)         // If there is an input file
    {
        input_file = fopen(filename, "r");
        if(input_file == NULL)
        {
            char error_message[30] = "An error has occurred\n";
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(1);
        }
    }
    else{
        input_file = stdin;
    }
    //**************************************************//
    char *command;
    char **arguments_array;
    char *exit_command = "exit\n";
    size_t size = 100;
    int line_size = 0;
    do{
        if(is_interactive)
            printf("ssh>>");
        line_size = getline(&command, &size, input_file);
        //********************************************************************//
        char* found;
        while(1)
        {
            arguments_array = calloc(50, sizeof(char*));
            found = strsep(&command, "&");          // Separate if multiple commands are passed in same line
            //***** if the first character is ' ' ******/
            if(found == NULL)
                break;
            //******************************************************************//
            if(found[0] == ' ' || found[0] == '\n')
                memmove(found, found + 1, strlen(found));
            unsigned long size_of_found = strlen(found);
            if(found[size_of_found-1] == ' ' || found[size_of_found-1] == '\n')
                found[size_of_found-1] = '\0';
            //******************************************************************//
            int num_of_args = parse(found, arguments_array);
            char* first_arg = arguments_array[0];
            if(first_arg == NULL)   // If nothing is typed in the input line and '\n' is the only character
                continue;
            if(is_built_in_command(first_arg))
            {
                run_built_in_commands(arguments_array, num_of_args, path_array, &paths_size);
            }
            else if(is_valid_system_command(first_arg, path_array, paths_size))
            {
                run_system_commands(arguments_array, num_of_args);
            }
            else{
                char error_message[30] = "An error has occurred\n";
                write(STDERR_FILENO, error_message, strlen(error_message));
            }
            for(int i = 0; i < num_of_args; i++)
            free(arguments_array[i]);
            free(arguments_array);
        }
        while ((wpid = wait(&status)) > 0);
    }while(line_size >= 0);
}
