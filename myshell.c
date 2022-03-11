#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/wait.h>
#include <fcntl.h>

//Error message that will be writen if an error happens
char error_message[30] = "An error has occurred\n";

//This is for background processing
//Since my background process returns the command line to the user
//we have to keep track of the process so we can exit it after the
//user enters the command
int background_process_loop = 0;

//Initializes the functions that will be used
char *read_user_input();
char **split_input(char *input);
int run_process(char **arguments, char **envp);
int my_cd(char **arguments);
int my_clr(char **arguments);
int my_dir(char **arguments);
int my_environ(char **arguments, char **envp);
int my_echo(char **arguments);
int my_help(char **arguments);
int my_pause(char **arguments);
int my_quit(char **arguments);
int external_process(char **arguments);
int my_pipe(char **write_args, char **read_args);
char * start;




//List of our built ins that we can loop through and check
//which one the user entered
char *builtin_command_list[] = {
    "cd",
    "clr",
    "dir",
    "echo",
    "help",
    "pause",
    "quit"

};
//Array of pointers to the functions
int (*builtin_command_functions[])(char **) = {
    &my_cd,
    &my_clr,
    &my_dir,
    &my_echo,
    &my_help,
    &my_pause,
    &my_quit};

//This is the main function in which the loop of user inputs will be handled
//Then the processing of the commands.
int main(int argc, char *argv[], char **envp)
{
    char *test;
    char **test_array;
    int status = 1;
    char shell[100] = "SHELL=";
    char *temp = getcwd(NULL, 0);
    temp = strcat(shell, temp);
    putenv(strcat(temp, "/myshell"));
    start = getcwd(NULL,0);


    


    if (argc == 1)
    { //If no other arguments it will work as a regular shell
        //We are seting the shell enviornment

        //Keeps the shell looping until exit
        while (status == 1)
        {
            printf("myshell~");
            printf("%s", getcwd(NULL, 0));
            printf("> ");
            //Reads user input
            test = read_user_input();
            //Splits it
            test_array = split_input(test);

            //runs whatever the input was
            run_process(test_array, envp);
        }
    }
    else if (argc == 2)
    { // if there is another argument it will be the batch file
        //It will loop through each line and read it as a command
        char buffer[100];
        FILE *fp = fopen(argv[1], "r");
        
        //Exits if there is no file by submitted name
        if (fp == NULL)
        {
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(1);
        }
        //While loop to loop through each command line by line
        while (fgets(buffer, 100, fp) != NULL)
        {
            //Splits the input by the file
            test_array = split_input(buffer);
            //then runs it
            run_process(test_array, envp);
        }
        //closes file
        fclose(fp);
    }
    else
    { //Error: If user enters too many arguments
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    }
    exit(0);
}

//This function will handle getting the user input and setting it to a char array
char *read_user_input()
{

    //Set the values we will use for get line
    char *user_input;
    ssize_t the_size = 0;

    //Gets the user input and sets it to the char array
    getline(&user_input, &the_size, stdin);

    //Then just returns the array
    return user_input;
}

//This is going to return the input split(by white spaces) into an array
char **split_input(char *input)
{

    //Initilize values
    int alloc_size = 50;
    char **seperated_input = malloc(alloc_size * sizeof(char *));
    char *token;
    int index = 0;

    //Allocation error catch
    if (!seperated_input)
    {
        write(STDERR_FILENO, error_message, strlen(error_message));
    }
    //Split the input by white space
    token = strtok(input, " \t\n");
    //Loop through tokens
    while (token != NULL)
    {
        //Sets the each token to an index
        seperated_input[index] = token;
        index++;
        //if seperated_input not big enough
        if (index >= alloc_size)
        {
            alloc_size += 50;
            seperated_input = realloc(seperated_input, alloc_size * sizeof(char *));
            if (!seperated_input)
            { //Memory allocation error
                write(STDERR_FILENO, error_message, strlen(error_message));
                return 0;
            }
        }
        //Move the token up one
        token = strtok(NULL, " \t\n");
    }
    //Then just set the last one to null
    seperated_input[index] = NULL;
    //Now return our sucessfully made parsed array of commands/args
    return seperated_input;
}
//This find out if its a built in or an external process and runs it as such
int run_process(char **arguments, char **envp)
{
    int delete_before = 0;
    int oldin = dup(0);
    int oldout = dup(1);
    int pid2 = 10;
    int multiout = 0;
    int multiin = 0;
    int multiout2 = 0;

    //Make sure there is an argument
    if (arguments[0] == NULL)
    {
        return 0;
    }

    //Now we are going to split it if their is file redirection
    //> case
    for (int i = 0; arguments[i] != NULL; i++)
    {
        //check if their is a set STDOUT
        if (strcmp(arguments[i], ">") == 0)
        {

            //Error if too many ">" in the command
            if (multiout == 0 && multiout2 == 0)
            {
                multiout = 1;
            }
            else
            {
                write(STDERR_FILENO, error_message, strlen(error_message));
                dup2(oldout, 1);

                dup2(oldin, 0);
                if (background_process_loop == 1) // This is to catch if its a background loop
                {
                    background_process_loop = 0;
                    exit(0);
                }
                return 0;
            }

            //make sure there is a file
            if (arguments[i + 1] != NULL)
            {
                //Makes file
                FILE *fp = fopen(arguments[i + 1], "w");
                fclose(fp);
                //if so we set STDOUT
                int myfile_descr = open(arguments[i + 1], O_RDWR);
                truncate(arguments[i + 1], 0);
                dup2(myfile_descr, STDOUT_FILENO);
            }
            else
            {
                //if not inform user
                write(STDERR_FILENO, error_message, strlen(error_message));
                dup2(oldout, 1);
                dup2(oldin, 0);
                if (background_process_loop == 1) // This is to catch if its a background loop
                {
                    background_process_loop = 0;
                    exit(0);
                }
                return 0;
            }
            //make sure to break it as early as possible
            if (delete_before > i || delete_before == 0)
            {
                delete_before = i;
            }
        }
        // < case
        if (strcmp(arguments[i], "<") == 0)
        {
            //Error if too many "<" are in the command line
            if (multiin == 0)
            {
                multiin = 1;
            }
            else
            {
                write(STDERR_FILENO, error_message, strlen(error_message));
                dup2(oldout, 1);
                dup2(oldin, 0);
                if (background_process_loop == 1) // This is to catch if its a background loop
                {
                    background_process_loop = 0;
                    exit(0);
                }
                return 0;
            }
            //if their is make sure their is a file
            if (arguments[i + 1] != NULL)
            {
                //if so we set STDIN
                FILE * fp = fopen(arguments[i+1],"r");
                if(fp == NULL){
                    write(STDERR_FILENO, error_message, strlen(error_message));
                dup2(oldout, 1);
                dup2(oldin, 0);
                if (background_process_loop == 1) // This is to catch if its a background loop
                {
                    background_process_loop = 0;
                    exit(0);
                }
                return 0;
                }
                fclose(fp);
                int myfile_descr = open(arguments[i + 1], O_RDWR);
                dup2(myfile_descr, STDIN_FILENO);
            }
            else
            {
                write(STDERR_FILENO, error_message, strlen(error_message));
                dup2(oldout, 1);
                dup2(oldin, 0);
                if (background_process_loop == 1) // This is to catch if its a background loop
                {
                    background_process_loop = 0;
                    exit(0);
                }
                return 0;
            }
            //make sure to break it as early as possible

            if (delete_before > i || delete_before == 0)
            {
                delete_before = i;
            }
        }
        // >> case
        if (strcmp(arguments[i], ">>") == 0)
        {
            if (multiout == 0 && multiout2 == 0)
            {
                multiout2 = 1;
            }
            else
            {
                write(STDERR_FILENO, error_message, strlen(error_message));
                dup2(oldout, 1);
                dup2(oldin, 0);
                if (background_process_loop == 1) // This is to catch if its a background loop
                {
                    background_process_loop = 0;
                    exit(0);
                }
                return 0;
            }
            //if their is make sure their is a file
            if (arguments[i + 1] != NULL)
            {
                //if so we set STDOUT
                int k = 0;
                char buffer[100];
                int alloc_size = 50;


                //Opens file i
                FILE *fp = fopen(arguments[i + 1], "r");
                //Exits if there is no file by submitted name
                if (fp == NULL)
                {

                    //Makes file
                    FILE *fp2 = fopen(arguments[i + 1], "w");
                    fclose(fp2);
                    //if so we set STDOUT
                    int myfile_descr = open(arguments[i + 1], O_RDWR);
                    truncate(arguments[i + 1], 0);
                    dup2(myfile_descr, STDOUT_FILENO);
                }
                else
                {

                    fclose(fp);
                    int myfile_descr = open(arguments[i + 1], O_RDWR | O_APPEND);
                    dup2(myfile_descr, STDOUT_FILENO);
                }
            }
            else
            {
                //if not inform user
                printf("invalid standard output");
            }
            //make sure to break it as early as possible
            if (delete_before > i || delete_before == 0)
            {
                delete_before = i;
            }
        }
        //Background Processing
        if (strcmp(arguments[i], "&") == 0)
        {
            if (multiin != 0 || multiout2 != 0 || multiout != 0)
            {
                write(STDERR_FILENO, error_message, strlen(error_message));
                dup2(oldout, 1);
                dup2(oldin, 0);
                if (background_process_loop == 1) // This is to catch if its a background loop
                {
                    background_process_loop = 0;
                    exit(0);
                }
                return 0;
            }
            //We are going to make a set of new_args which will be everything
            //To the right of the &
            char **new_arguments = malloc(50 * sizeof(char *));
            int k = i;
            int l = 0;
            //Sets the delete before so it can split it later
            if (delete_before > i || delete_before == 0)
            {
                delete_before = i;
            }
            //Puts a set of new arguments after the index of &
            for (k += 1; arguments[k] != NULL; k++)
            {

                new_arguments[l] = arguments[k];
                l++;
            }
            //Creates the intial fork for the background processes to run in
            pid2 = fork();
            if (pid2 < 0)
            {
                write(STDERR_FILENO, error_message, strlen(error_message));
                dup2(oldout, 1);
                dup2(oldin, 0);
                if (background_process_loop == 1) // This is to catch if its a background loop
                {
                    background_process_loop = 0;
                    exit(0);
                }
                return 0;
            }
            else if (pid2 == 0)
            {

                //This is a fork for both processes
                int pid3 = fork();
                if (pid3 < 0)
                {
                    write(STDERR_FILENO, error_message, strlen(error_message));
                    dup2(oldout, 1);
                    dup2(oldin, 0);
                    if (background_process_loop == 1) // This is to catch if its a background loop
                    {
                        background_process_loop = 0;
                        exit(0);
                    }
                    return 0;
                }
                else if (pid3 == 0)
                { //Runs anything right of the &
                    arguments[i + 1] = NULL;
                    arguments[i] = NULL;
                    run_process(arguments, envp);

                    exit(0);
                }
                else
                { //Runs anything left of the &
                    if (new_arguments[0] != NULL)
                    { //If there is another command run it right away
                        run_process(new_arguments, envp);
                    }
                    else
                    {   //If there isn't another command
                        //Give input back to the user and keep track that after
                        //the user enters a new command to run it and exit
                        background_process_loop = 1;
                        return 0;
                    }

                    waitpid(pid3, NULL, 0);
                    exit(0);
                }
            }
            else
            {
                //then waits for branches to finish before giving input back to user

                waitpid(pid2, NULL, 0);
                printf("Background processes has finished \n");

                return 0;
            }
        }
        //pipe case

        if (strcmp(arguments[i], "|") == 0)
        {

            int status;
            int k = i;
            char **temp_args = malloc(50 * sizeof(char *));
            int pipeargs[2];
            int l = 0;
            arguments[i] = NULL;
            if (pipe(pipeargs) == -1)
            {
                write(STDERR_FILENO, error_message, strlen(error_message));
                dup2(oldout, 1);
                dup2(oldin, 0);
                if (background_process_loop == 1) // This is to catch if its a background loop
                {
                    background_process_loop = 0;
                    exit(0);
                }
                return 0;
            }
            for (k += 1; arguments[k] != NULL; k++)
            {

                temp_args[l] = arguments[k];
                l++;
                temp_args[k] = NULL;
            }

            temp_args[l] = NULL;
            my_pipe(arguments, temp_args);
            dup2(oldout, 1);
            dup2(oldin, 0);
            if (background_process_loop == 1) // This is to catch if its a background loop
            {
                background_process_loop = 0;
                exit(0);
            }
            return 0;
        }
    }

    //Kinda resets arguments
    if (delete_before != 0)
    {
        arguments[delete_before] = NULL;
    }

    //Loop through them to see if their is a built in
    for (int i = 0; i <= 6; i++)
    {
        if (strcmp(arguments[0], builtin_command_list[i]) == 0)
        {
            (*builtin_command_functions[i])(arguments);
            dup2(oldout, 1);
            dup2(oldin, 0);
            if (background_process_loop == 1) // This is to catch if its a background loop
            {
                background_process_loop = 0;
                exit(0);
            }
            return 0;
        }
    }
    //Had to make a special case for environ
    if (strcmp(arguments[0], "environ") == 0)
    {
        my_environ(arguments, envp);

        dup2(oldout, 1);
        dup2(oldin, 0);
        if (background_process_loop == 1) // This is to catch if its a background loop
        {
            background_process_loop = 0;
            exit(0);
        }
        return 0;
    }

    external_process(arguments);
    //Fixes the STDIN and STDOUT
    dup2(oldout, 1);
    dup2(oldin, 0);
    if (background_process_loop == 1) // This is to catch if its a background loop
    {
        background_process_loop = 0;
        exit(0);
    }

    return 0;
}
//When called changes the directory to argument after cd
int my_cd(char **arguments)
{
    int check;
    //Check to see if they pass another argument
    if (arguments[1] == NULL)
    {
        //If they did not print current directory
        fprintf(stdout, "%s\n", getcwd(NULL, 0));
        //Now if their is another argument
    }
    else if (arguments[2] == NULL)
    {
        //We change directory to that argument
        check = chdir(arguments[1]);
        //Check if the change was sucsessful
        if (check < 0)
        {
            write(STDERR_FILENO, error_message, strlen(error_message));
            return 0;
        }
        //Check if the user passed to many arguments
    }
    else
    {
        write(STDERR_FILENO, error_message, strlen(error_message));
        return 0;
    }

    return 0;
}
//When called clears the terminal
int my_clr(char **arguments)
{
    if (arguments[1] != NULL)
    {
        write(STDERR_FILENO, error_message, strlen(error_message));
        return 0;
    }
    printf("\033[H\033[2J");
    return 0;
}
//This will either print the current contents directory or if passed with and argument
//print the contents of the argument
int my_dir(char **arguments)
{
    //Sets up all the directory values we need
    DIR *x;
    struct dirent *directory_pointer;

    //Check if an arguement is passed
    //If no arguments
    if (arguments[1] == NULL)
    {
        //gets our directory
        x = opendir(getcwd(NULL, 50));
        //Error catch
        if (x == NULL)
        {
            write(STDERR_FILENO, error_message, strlen(error_message));
            return 0;
        }
        //Loops through and prints our directory
        while ((directory_pointer = readdir(x)) != NULL)
        {
            fprintf(stdout, "%s/%s\n", getcwd(NULL, 50), directory_pointer->d_name);
        }
        //If argument is passed
    }
    else if (arguments[2] == NULL)
    {
        //Same thing happens except we open argument[1]
        x = opendir(arguments[1]);
        if (x == NULL)
        {
            write(STDERR_FILENO, error_message, strlen(error_message));
            return 0;
        }
        while ((directory_pointer = readdir(x)) != NULL)
        {
            fprintf(stdout, "%s/%s\n", arguments[1], directory_pointer->d_name);
        }
        //Error catch for to many arguments
    }
    else
    {
        write(STDERR_FILENO, error_message, strlen(error_message));
        return 0;
    }

    return 0;
}
//This function when called will print out all evironment strings
int my_environ(char **arguments, char **envp)
{
    if (arguments[1] != NULL)
    {
        write(STDERR_FILENO, error_message, strlen(error_message));
        return 0;
    }
    //Iterates through environment variables with the pointer temp
    //Just move up the pointer every loop until it doesnt point to anything
    for (char **temp = envp; *temp != 0; temp++)
    {
        //Print out each element from the environment list
        fprintf(stdout, "%s\n", *temp);
    }
    return 0;
}
//When called will print whatever is to the right of it
int my_echo(char **arguments)
{

    //Iterate through arguments
    char **temp;
    //This just jumps past echo
    temp = arguments;
    temp++;

    for (; *temp != 0; temp++)
    {
        //Print out each element from the environment list
        fprintf(stdout, "%s ", *temp);
    }
    fprintf(stdout, "\n");
    return 0;
}
//When called prints out the help text document
int my_help(char **arguments)
{
    if (arguments[1] != NULL)
    {
        write(STDERR_FILENO, error_message, strlen(error_message));
        return 0;
    }
    char buffer[100];
    char location2[100];
    char temp_start[100];
    strcpy(temp_start, start);
    FILE *fp = fopen(strcat(temp_start,"/readme_doc"), "r");
    //Exits if there is no file by submitted name
    if (fp == NULL)
    {
        write(STDERR_FILENO, error_message, strlen(error_message));
        return 0;
    }

    //While loop to print file line by line
    while (fgets(buffer, 100, fp) != NULL)
    {
        fprintf(stdout, "%s\n", buffer);
    }
    //closes file
    fclose(fp);
    return 0;
}
//When called it pauses the terminal until enter is pressed
int my_pause(char **arguments)
{
    if (arguments[1] != NULL)
    {
        write(STDERR_FILENO, error_message, strlen(error_message));
        return 0;
    }
    char x;
    while (x != '\n')
    {
        x = getchar();
    }
    return 0;
}

//This ends the whole shell program when called
int my_quit(char **arguments)
{
    if (arguments[1] != NULL)
    {
        write(STDERR_FILENO, error_message, strlen(error_message));
        return 0;
    }
    exit(0);
}
//This handles if the user enters a command that is not built in
int external_process(char **arguments)
{
    int pid;
    int status;
    int result;

    pid = fork();
    //Error making fork
    if (pid < 0)
    {
        if (arguments[1] != NULL)
        {
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(0);
        }
    }
    else if (pid == 0)
    { //check if child
        //If it is execute the command/process
        if (execvp(arguments[0], arguments) < 0)
        {
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(0);
        }
    }
    else
    {
        //Parent waits for child to execute
        result = wait(&status);
    }
    return 0;
}
//This take in two arguments
//-write_args: The command that's output will be sent to read_args
//-read_args: Take the output of the write_args and uses it in its own command
//Like "help | wc" will print out the word count of what help prints
int my_pipe(char **write_args, char **read_args)
{
    //First we need to set up the pip arguments
    int pipeargs[2];
    //Then pass them into the pipe function
    pipe(pipeargs);
    //We are going to fork and run one command in parent and one in child
    pid_t pid = fork();
    //Error making fork
    if (pid < 0)
    {
        write(STDERR_FILENO, error_message, strlen(error_message));
        return 0;
    }
    else if (pid == 0)
    { //Child: will run the write_args
        //First off we set the pipeargs to STDOUT
        dup2(pipeargs[1], STDOUT_FILENO);
        //Close the unused output things
        close(pipeargs[0]);
        close(pipeargs[1]);
        //Then execute the command
        if (execvp(write_args[0], write_args) < 0)
        { //Error case: Command doesn't work

            write(STDERR_FILENO, error_message, strlen(error_message));
            return 0;
        };
    }
    else
    { //Parent
        //Pretty much the same thing now with the read_args
        pid = fork();
        if (pid < 0)
        {
            write(STDERR_FILENO, error_message, strlen(error_message));
            return 0;
        }
        else if (pid == 0)
        { //Child
            //Set the STDIN to the pipeargs
            dup2(pipeargs[0], STDIN_FILENO);
            //Again close the things we don't use
            close(pipeargs[1]);
            close(pipeargs[0]);
            //Then execute the command
            //Put error case
            execvp(read_args[0], read_args);
        }
        else
        { //Parent waits for child and ccloses the pipeargs
            int status;
            close(pipeargs[1]);
            close(pipeargs[0]);
            waitpid(pid, &status, 0);
        }
    }
    return 0;
}