#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>
#include <cstring>
#include <unistd.h>//The isatty() function shall return 1 if fildes is associated with a terminal; otherwise, it shall return 0 and may set errno to indicate the error.

using namespace std;

#define Max_Arg_Size 32
#define Max_Input_Size 512


int len(char** array) {
    int length = 0;

    while (1) {
        if (array[length] == NULL) { break; }
        length++;
    }

    return length;
}

void input_redirect(char** args) {
    int i = 0;
    char* token;
    while(token = args[++i]){
        if(strcmp(token, "<") == 0) { // if current token == "<"
            int in = open(args[i+1], O_RDONLY, 0777); // then the next token will be the infile
        	//forget to check if in is vaild? 
            dup2(in, 0); // redirect stdin to be the file
            args[i] = NULL; // signals the command ends here
        }
    }

}

static int child = 0; /* whether it is a child process relative to main() */

void runcommand(char** args) {
    int i = 0; // current token counter
    int save_in = dup(0); // save the stdin and stdout
    int save_out = dup(1);

    char* token;
    while(token = args[++i]){
        if(strcmp(token, ">") == 0) { // if current token == ">"
            int out = open(args[i+1], O_TRUNC | O_CREAT | O_WRONLY, 0777); // then the next token will be the outfile
            dup2(out, 1); // redirect stdout to be the file
            args[i] = NULL; // signals the command ends here
        }
    }

    pid_t pid = fork();
    if(pid) { // parent
        waitpid(pid, NULL, 0);
    } 
    else { // child
        execvp(args[0], args);
        perror("ERROR"); // if execvp returns, the command fails
		exit(0);
    }
    dup2(save_in, 0); dup2(save_out, 1); // restore the stdin and stdout
    close(save_in); close(save_out); // close the saver
}

static void exec_pipeline(char*** cmds, size_t pos) {
    if (cmds[pos + 1] == NULL) { /* last command */

        runcommand(cmds[pos]);
    }
    else {
        int fd[2];
        pipe(fd);
        pid_t pid = fork();
        switch(pid) {
            case -1:
                printf("ERROR: fork failed!\n");
            case 0:
                child = 1;

                close(fd[0]);
                dup2(fd[1],1);
                close(fd[1]);

                input_redirect(cmds[pos]);
                execvp(cmds[pos][0], cmds[pos]);
				perror("ERROR");
				exit(0);
            default:
                close(fd[1]);
                dup2(fd[0],0);
                close(fd[0]);
                wait(NULL);
                exec_pipeline(cmds, pos + 1);
        }
    }
}

vector<string> tokenizeInput(string inputString)
{
    vector<string> result;
    istringstream ss1(inputString);
    string temp;
    string first_parse;
    while(getline(ss1,temp,'\n')){
        istringstream temp1(temp);
        while(getline(temp1,temp,'\f')){
            istringstream temp2(temp);
            while(getline(temp2,temp,'\r')){
                istringstream temp3(temp);
                while(getline(temp3,temp,'\t')){
                    istringstream temp4(temp);
                    while(getline(temp4,temp,'\v')){
                        first_parse = first_parse + temp;
                    }
                }
            }
        }
    }
    istringstream ss2(first_parse);
    const string delims("<>|&");
    while(getline(ss2,temp,' ')){
        string temp2="";
        for(unsigned int i=0;i<temp.length();i++)
        {
            if(temp[i] != '|' && temp[i] != '<' && temp[i] != '>' && temp[i] != '&'){
                temp2=temp2+temp[i];
            }
            else{
                result.push_back(temp2);
                string temp3 = "";
                temp3 = temp3 + temp[i];
                result.push_back(temp3);
                temp2.clear();
            }
        }
        result.push_back(temp2);

    }
    for(unsigned int j=0; j<result.size();j++){
        if(result[j].length() == 0){
            result.erase(result.begin()+j);
        }
    }
    return result;

}

int findIndex(vector<string> inputCMD, const char inputSym) {
    int i;

    for(int i=0;i<inputCMD.size();i++){
        if(inputCMD[i].c_str()[0]==inputSym){
            return i;
        }
    }
    return -1;
}
//input:ex ls ws | sort | wc
//deletePartial(input, 2, 5);
//output:ls ws
vector<string> deletePartial(vector<string> inputCMD, int start, int end){
    vector<string> result;
    int len = inputCMD.size();
    for(int i=0; i<len; i++){
        if(i<start || i>end){
            result.push_back(inputCMD[i]);
        }
    }
    return result;

}

//count pipes
int numSym(vector<string> inputCMD, const char inputSym){
    int countSym = 0;
    for(int i=0;i<inputCMD.size();i++){
        if(inputCMD[i].c_str()[0]==inputSym){
            countSym++;
        }
    }
    return countSym;
}


void inputRedirect(const char *filename){
    int in_fd=open(filename, O_RDONLY);
    //close(fileno(stdin));
    //dup2(in_fd,fileno(stdin));
    dup2(in_fd,0);//input file descriptor is 0
    close(in_fd);

}

void outputRedirect(const char *filename){
    int out_fd=open(filename, O_CREAT|O_WRONLY|O_TRUNC,0600);
    //close(fileno(stdout));
    //dup2(out_fd,fileno(stdout));
    dup2(out_fd,1);//output file descriptor is 1
    close(out_fd);

}

void func_kill_zombie(int signum){
  pid_t pid;
   pid =  wait(NULL);
   cout << pid << " exited" << endl;
}


void execuateCMD(vector<string>& inputCMD, bool isWait){
    int numPipes = numSym(inputCMD,'|');
    //NO PIPES, BASIC CMD
    if(numPipes==0){
        //handle cat < x > y when handling basic cmds like cat < x and cat < y
        pid_t pid = fork();
        if(pid<0){
            printf("ERROR: fork failed!\n");
        }
        //child process
        else if(pid==0){
            int inDirIdx = findIndex(inputCMD, '<');
            if(inDirIdx != -1){
                inputRedirect(const_cast<char*>(inputCMD[inDirIdx+1].c_str()));
                inputCMD = deletePartial(inputCMD,inDirIdx,inDirIdx+1);
              }
              int outDirIdx = findIndex(inputCMD,'>');
              if(outDirIdx != -1){
                //unknown error
                outputRedirect(const_cast<char*>(inputCMD[outDirIdx+1].c_str()));
                inputCMD = deletePartial(inputCMD,outDirIdx,outDirIdx+1);
              }
              //convert string to char**
              vector<char*> tmp;
              char** command = new char*[inputCMD.size()+1];
              for(unsigned int i=0;i<inputCMD.size();i++){
                command[i] = new char[inputCMD[i].size()+1];
                strcpy(command[i],const_cast<char*>(inputCMD[i].c_str()));
              }
              command[inputCMD.size()] = (char*)0;
              execvp(command[0],command);
              perror("ERROR");
              exit(0);

        }
        //parent
        else{
            //& wait for child;
            if(isWait){
              wait(NULL);
            }
            else{
            	//waitpid(-1,0,WNOHANG);
            }
        }
    }
    //handle pipes
    else{
        vector< vector<string> > new_CMD;
        for(int i=0;i<numPipes;i++){
            new_CMD.push_back(deletePartial(inputCMD,findIndex(inputCMD,'|'),inputCMD.size()-1));
            inputCMD = deletePartial(inputCMD,0,findIndex(inputCMD,'|'));
        }
        new_CMD.push_back(inputCMD);

        vector<char**>new_CMD_char;
        for(unsigned int i=0;i<new_CMD.size();i++){
            char** command = new char*[new_CMD[i].size()+1];
            for(unsigned int j=0;j<new_CMD[i].size();j++){
                command[j] = new char[new_CMD[i][j].size()+1];
                strcpy(command[j],const_cast<char*>(new_CMD[i][j].c_str()));
            }
        command[new_CMD[i].size()] = (char*)0;
        new_CMD_char.push_back(command);
        }
        char*** cmds = new char**[new_CMD_char.size()];
        for(unsigned int i=0;i<new_CMD_char.size();i++){
                cmds[i]=new_CMD_char[i];

        }
        cmds[new_CMD_char.size()] = (char**)0;


        pid_t pid = fork();

        if(pid<0){
            printf("ERROR: fork failed!\n");
        }
        else if(pid==0){
            //new_CMD_char.erase(new_CMD_char.begin());
            //for(unsigned int i=0;i<new_CMD_char.size();i++){
                /*for(unsigned int j=0;j<len(new_CMD_char[i]);j++){
                    cout<<new_CMD_char[i][j]<<' ';
                }

                cout<<endl;*/
                //runpipe2(new_CMD_char[0],new_CMD_char[1],new_CMD_char[2]);
                exec_pipeline((char***)cmds, 0);
        }
        //parent
        else{
        	//signal(SIGCHLD, func_kill_zombie);
            if(isWait){
            	wait(NULL);
            }
            else{
	      		//signal(SIGCHLD, func_kill_zombie);
            }

        }
    }
}


int main(int argc, char *argv[]) {
    string input;
    char inputLine[Max_Input_Size];
    vector <string> args;
    bool flag_auto = true;
    
    if(argc > 1 && strcmp(argv[1],"-n")==0)
        flag_auto = false;

    while(1) {
    	        //signal(SIGCHLD, func_kill_zombie);
    	waitpid(-1,0,WNOHANG);
        if (isatty(STDIN_FILENO) && flag_auto) {
            printf("shell: ");
        }
        if(fgets(inputLine, Max_Input_Size, stdin)){

        input =string(inputLine);
        args = tokenizeInput(input);

        if(args.size()==0){
            printf("ERROR: Empty Command!\n");
        }
        else{
            if(args[0]=="exit"){
                exit(0);
            }
            else if(args[0]=="cd"){
                chdir(args[1].c_str());
            }
            else{
                bool isWait = true;
                if(args[args.size()-1]=="&"){
                    isWait=false;
                    args.erase(args.end());
                }
                execuateCMD(args,isWait);
            }
        }
        //for (auto i = args.begin(); i != args.end(); ++i)
        //    cout << *i << '\n';


        //if (!isatty(STDIN_FILENO)) { exit(1); }
    }
    else{
      exit(0);
    }
  }

    //In UNIX and Linux, at least, ^D sends an EOF over stdin. So you really don't have to do anything fancy; just read stdin until you get an EOF:

    return 0;
}
