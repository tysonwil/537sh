//
//  537sh.c
//
//
//  Created by Tyson Williams on 2/9/13.
//
//  parsing of commands into name, argc, argv[] is complete
//  need to test implementations of cd, pwd, exit (built in commands)
//  implement non-built in commands with fork()
//  figure out a way to run processes in parallel and sequential --> when to run them based on how they were broken down
//  file output redirection with ">" character
//  interactive mode (only batch is supported now)
//

/*
 Disc notes
 --> handling exit when in parallel
 if it ends in plus, dont wait
 if it doesnt end in plus -- wait
 only check for exit if it doesnt end in a plus
 int childStatus
 
 childPID = fork();
 if (child) {#include <fcntl.h>
 exit_cmd:
 exit(1);
 else (parent)
 waitpid(childPID, &childStatus, WUNTRACED)
 WEXITSTATUS(childStaus)
 
 ls; cd; echo catfish
 1. fork a child process
 2. execvp(name, argv)
 we need to fork a new process if it is not built in
 
 */

#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define WHITESPACE " \t\n"
#define OTHER_CMD 0
#define CD_CMD    1
#define PWD_CMD   2
#define EXIT_CMD  3

// the structure of a command: name, argc, argv[]
struct command_t {
    char * name;
    int argc;
    char * argv[257];
    int file_redirect;
    char * outputFile;
};

struct command {
    char * words;
    char * outputFile;
};

struct sequence {
    struct command_t * commandArray;
    char * commands;
    struct command * cmd;
    int numCommands;
};

struct line {
    struct sequence * sequences;
    int isEndedInPlus;
    int numSequences;
    
    
};

// each built-in command has a name and and an identifier
typedef struct built_in_commands built_in_commands;
struct built_in_commands{
    char command_name[512];
    int command_code;
};

// define built-in commands and their codes
built_in_commands my_commands[3] = {
    {"exit" , EXIT_CMD},
    {"pwd"  , PWD_CMD },
    {"cd"   , CD_CMD  }
};

//
// Function declarations#include <fcntl.h>
//
void parse_command (char *cmdline, struct command_t *cmd);
int get_cmd_code (char* cmd_name);
void do_cd (char * path);
char *trimwhitespace(char *str);
int count_chars(const char* string, char ch);
void runCommand(struct command_t *cmdHead);
void runSequence(struct sequence *mySequence);

// Global variables
char error_message[30] = "An error has occurred\n";
struct command_t cmdHead = {NULL, 0, {NULL}};

void runCommand(struct command_t *cmdHead)
{
    char * direct;
    direct = (char*) malloc(512);
    int status;
    int pid;
    
    switch (get_cmd_code (cmdHead->name)) {
            
        case EXIT_CMD:
            exit(0);
        case CD_CMD:
            do_cd(cmdHead->argv[1]);
            break;
        case PWD_CMD:
            if (cmdHead->argc > 1) {
                write(STDERR_FILENO, error_message, strlen(error_message));
                break;
            }
            direct = getcwd(direct, 512);
            write(STDOUT_FILENO, direct, strlen(direct));
            write(STDOUT_FILENO, "\n", 1);
            break;
        case OTHER_CMD:
            pid = fork();
            
            // fork succeeded
            if (pid >= 0) {
                // fork() returns 0 to the child process
                if (pid == 0) {
                    if (cmdHead->file_redirect == 1) {
                        int out = open(cmdHead->outputFile, O_RDWR | O_CREAT , 0666 );
                        if (out == -1) {
                            write(STDERR_FILENO, error_message, strlen(error_message));
                            break;
                        }
                        dup2(out, STDOUT_FILENO);
                    }
                    //printf("child process\n");
                    //printf("HERE: %s\n",cmdHead->name);
                    execvp (cmdHead->name, cmdHead->argv);
                    // If execvp returns, it must have failed
                    write(STDERR_FILENO, error_message, strlen(error_message));
                    exit(1);
                }
                // fork() returns new pid to parent process
                else {
                    //printf("parent process\n");
                    wait(&status);
                }
            }
            // fork() returns -1 on failure
            else {
                write(STDERR_FILENO, error_message, strlen(error_message));
                exit(0);
            }
    }
}

// parses the command into a command_t struct
// assigns name, argc, and argv[] to prepare for execution
void parse_command (char *cmdline, struct command_t *cmd)
{
    int count = 0;
    char * word;
    char * out;
    out = (char*) malloc(512);
    cmd->file_redirect = 0;
    
    if (count_chars(cmdline, '>') == 1) {
        cmd->file_redirect = 1;
        word = strtok (cmdline, ">");
        out = strtok (NULL, ">");
        out = trimwhitespace(out);
        
        cmd->outputFile = out;
    }
    
    word = strtok (cmdline, WHITESPACE);
    
    if (word == NULL) { word = ""; } // Fixes blank line bug
    
    while (word) {
        cmd->argv[count] = word;
        word = strtok (NULL, WHITESPACE);
        count++;
    }
    cmd->argv[count] = NULL;
    cmd->argc = count;
    cmd->name = (char *) malloc (strlen (cmd->argv[0])+1);
    strcpy (cmd->name, cmd->argv[0]);
}

int get_cmd_code (char* cmd_name)
{
    int i = 0;
    for (i=0; i<3; i++){
        if (!strcmp (my_commands[i].command_name, cmd_name))
            return my_commands[i].command_code;
    }
    return OTHER_CMD;
}

// change directory
// if no arguments givem, change to $HOME
void do_cd (char * path)
{
    char error_message[30] = "An error has occurred\n";
    if (path == NULL) {
        int cd = chdir(getenv("HOME"));
        if (cd == 1)
            write(STDERR_FILENO, error_message, strlen(error_message));
        return;
    }
    
    if (chdir (path) == 0) {
        getcwd (path, 512);
        setenv ("PWD", path, 1);
    } else {
        write(STDERR_FILENO, error_message, strlen(error_message));
    }
}

char *trimwhitespace(char *str)
{
    char *end;
    
    // Trim leading space
    while(isspace(*str)) str++;
    
    if(*str == 0) {
        // All spaces?
        //str = NULL;
        return str;
    }
    
    // Trim trailing space
    end = str + strlen(str) - 1;
    while(end > str && isspace(*end)) end--;
    
    // Write new null terminator
    *(end+1) = 0;
    
    return str;
}

// return the number of times ch appears in string
int count_chars(const char* string, char ch)
{
    int count = 0;
    for(; *string; count += (*string++ == ch)) ;
    return count;
}

int main(int argc, char *argv[])
{
    char *commandInput;
    commandInput = (char*) malloc(1000);
    
    if (argc > 2) {
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    }
    
    if (argc == 2) {
        FILE *batchFile;
        
        batchFile = fopen(argv[1], "r");
        if (batchFile == NULL) {
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(1);
        }
        else {
            while (fgets(commandInput, 1000, batchFile) != NULL) {
                int length = strlen(commandInput);
                if (length > 512) {
                    write(STDERR_FILENO, error_message, strlen(error_message));
                    write(STDOUT_FILENO, commandInput, strlen(commandInput));
                    fgets(commandInput, 1000, batchFile);
                }
                
                write(STDOUT_FILENO, commandInput, strlen(commandInput));
                //printf("\n");
                
                char *commandInputTrimmed;
                commandInputTrimmed = (char*) malloc(512);
                commandInputTrimmed =  trimwhitespace(commandInput);
                
                //create a new line
                struct line myLine;
                
                if(commandInputTrimmed[strlen(commandInputTrimmed)-1] == '+')
                    myLine.isEndedInPlus = 1;
                else
                    myLine.isEndedInPlus = 0;
                
                // create the sequences (a "+" between each sequence)
                // we remove one if the line ends by a '+'
                int numSeq = count_chars(commandInputTrimmed, '+');
                if (myLine.isEndedInPlus == 0)
                    numSeq++;
                
                //an array of sequences
                struct sequence *sequencesInMyLine;
                sequencesInMyLine = (struct sequence*) malloc(numSeq);
                
                
                int counter = 0;
                char *token;
                token = (char*) malloc(512);
                token = strtok( commandInputTrimmed, "+" );
                //printf("TOKEN:%s\n",token);
                
                while( token != NULL ) {
                    token = trimwhitespace(token);
                    //printf( "sequence is \"%s\"\n", token);
                    if (*token != '\0') {
                        sequencesInMyLine[counter].commands = token;
                        counter++;
                    }
                    token = strtok( NULL, "+" );
                    
                }
                myLine.numSequences = counter;
                
                // link the sequences to the line
                myLine.sequences = sequencesInMyLine;
                
                int i;
                int c1;
                for (i=0; i<myLine.numSequences; i++) {
                    int numCmd = count_chars(sequencesInMyLine[i].commands, ';');
                    
                    struct command * commandsInMySequence;
                    commandsInMySequence = (struct command*) malloc(numCmd+1);
                    
                    sequencesInMyLine[i].cmd = commandsInMySequence;
                    char * commandToken;
                    token = (char*) malloc(512);
                    
                    c1 = 0;
                    
                    commandToken = strtok( sequencesInMyLine[i].commands, ";");
                    
                    while( commandToken != NULL) {
                        commandToken = trimwhitespace(commandToken);
                        if (*commandToken != '\0') {
                            sequencesInMyLine[i].cmd[c1].words = commandToken;
                            //printf("command is %s\n", sequencesInMyLine[i].cmd[c1].words);
                            c1++;
                        }
                        commandToken = strtok( NULL, ";");
                    }
                    sequencesInMyLine[i].numCommands = c1;
                    //printf("C1: %d\n",c1);
                }
                
                
                
                int z;
                int t;
                int commandArraySize;
                
                for (t=0; t < myLine.numSequences; t++) {
                    commandArraySize = sequencesInMyLine[t].numCommands;
                    struct command_t cmdArray[commandArraySize];
                    sequencesInMyLine[t].commandArray = cmdArray;
                    
                    for (z=0; z < commandArraySize; z++) {
                        parse_command(sequencesInMyLine[t].cmd[z].words, &sequencesInMyLine[t].commandArray[z]);
                        //printf("command name: %s\n",sequencesInMyLine[t].commandArray[z].name);
                        //printf("argc: %d\n",sequencesInMyLine[t].commandArray[z].argc);
                        //printf("argv[1]: %s\n",sequencesInMyLine[t].commandArray[z].argv[1]);
                        
                        runCommand(&sequencesInMyLine[t].commandArray[z]);
                    }
                }
            }
            fclose(batchFile);
        }
    }
    
    if (argc == 1) {
        char shellCommandMessage[30] = "537sh$ ";
        
        while (1) {
            write(1, shellCommandMessage, strlen(shellCommandMessage));
            
            fgets(commandInput, 1000, stdin);
            int length = strlen(commandInput);
            if (length > 512) {
                write(STDERR_FILENO, error_message, strlen(error_message));
                write(STDOUT_FILENO, commandInput, strlen(commandInput));
                fgets(commandInput, 1000, stdin);
            }
            
            
            char *commandInputTrimmed;
            commandInputTrimmed = (char*) malloc(512);
            commandInputTrimmed =  trimwhitespace(commandInput);
            
            //create a new line
            struct line myLine;
            
            if(commandInputTrimmed[strlen(commandInputTrimmed)-1] == '+')
                myLine.isEndedInPlus = 1;
            else
                myLine.isEndedInPlus = 0;
            
            // create the sequences (a "+" between each sequence)
            // we remove one if the line ends by a '+'
            int numSeq = count_chars(commandInputTrimmed, '+');
            if (myLine.isEndedInPlus == 0)
                numSeq++;
            
            //an array of sequences
            struct sequence *sequencesInMyLine;
            sequencesInMyLine = (struct sequence*) malloc(numSeq);
            
            
            int counter = 0;
            char *token;
            token = (char*) malloc(512);
            token = strtok( commandInputTrimmed, "+" );
            //printf("TOKEN:%s\n",token);
            
            while( token != NULL ) {
                token = trimwhitespace(token);
                //printf( "sequence is \"%s\"\n", token);
                if (*token != '\0') {
                    sequencesInMyLine[counter].commands = token;
                    counter++;
                }
                token = strtok( NULL, "+" );
                
            }
            myLine.numSequences = counter;
            
            // link the sequences to the line
            myLine.sequences = sequencesInMyLine;
            
            int i;
            int c1;
            for (i=0; i<myLine.numSequences; i++) {
                int numCmd = count_chars(sequencesInMyLine[i].commands, ';');
                
                struct command * commandsInMySequence;
                commandsInMySequence = (struct command*) malloc(numCmd+1);
                
                sequencesInMyLine[i].cmd = commandsInMySequence;
                char * commandToken;
                token = (char*) malloc(512);
                
                c1 = 0;
                
                commandToken = strtok( sequencesInMyLine[i].commands, ";");
                
                while( commandToken != NULL) {
                    commandToken = trimwhitespace(commandToken);
                    if (*commandToken != '\0') {
                        sequencesInMyLine[i].cmd[c1].words = commandToken;
                        //printf("command is %s\n", sequencesInMyLine[i].cmd[c1].words);
                        c1++;
                    }
                    commandToken = strtok( NULL, ";");
                }
                sequencesInMyLine[i].numCommands = c1;
                //printf("C1: %d\n",c1);
            }
            
            
            
            int z;
            int t;
            int commandArraySize;
            
            for (t=0; t < myLine.numSequences; t++) {
                commandArraySize = sequencesInMyLine[t].numCommands;
                struct command_t cmdArray[commandArraySize];
                sequencesInMyLine[t].commandArray = cmdArray;
                
                for (z=0; z < commandArraySize; z++) {
                    parse_command(sequencesInMyLine[t].cmd[z].words, &sequencesInMyLine[t].commandArray[z]);
                    //printf("command name: %s\n",sequencesInMyLine[t].commandArray[z].name);
                    //printf("argc: %d\n",sequencesInMyLine[t].commandArray[z].argc);
                    //printf("argv[1]: %s\n",sequencesInMyLine[t].commandArray[z].argv[1]);
                    
                    runCommand(&sequencesInMyLine[t].commandArray[z]);
                }
            }
        }
    }
    
    
    
    return 0;
}
