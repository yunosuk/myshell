#include <iostream>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>

void vectorSplit(std::vector<std::string>& vec, std::string s, char c);
void doCommand(std::vector<std::string> vec);
int ForkCommand(std::vector<std::string> vec);
void TernaryOperator(std::vector<std::string> first_commands, std::vector<std::string> commands);
void Corpus(std::vector<std::string> vec);

volatile sig_atomic_t sigint = 0;
static struct sigaction sa_sigint;

void sigint_handler(int signum) {
    sigint = 1;
    std::cout << "\n" << "^C (SIGINT caught!)" << "\n";
}

int main(){
    int count = 0;
    sa_sigint.sa_handler = sigint_handler;
    if (sigaction(SIGINT, &sa_sigint, NULL) < 0) {
        perror("sigaction");
        return -1;
    }
    while(1){
        sigint = 0;
        std::cout << "./myshell[" << count << "]" << ">";
        std::string s;
        std::getline(std::cin, s);
        if(sigint == 1){
            std::cin.clear();
            count++;
            continue;
        }
        if(std::cin.eof()){
            std::cout << "\n";
            std::cout << "bye" << "\n";
            exit(0);
        }
        std::vector<std::string> vec;
        vectorSplit(vec, s, ' ');
        // コーパスの確認
        Corpus(vec);
        // <--コーパスの確認
        count++;
    }
}

void vectorSplit(std::vector<std::string>& vec, std::string s, char c){
    int string_size = s.length();
    std::string ele = "";
    for(int i = 0; i < string_size; i++){
        if(s[i] == c){
            if (ele != ""){
                 vec.push_back(ele);
            }
            ele = "";
        } else {
            ele += s[i];
        }
        if(i == s.length() - 1){
            if (ele != ""){
                 vec.push_back(ele);
            }
        }
    }
}

void splitPipe(std::vector<std::string> vec, std::vector< std::vector<std::string> >& commands){
    int cnt = 0;
    std::vector<std::string> com;
    commands.push_back(com);
    for(int i = 0; i < vec.size(); i++){
        if(vec[i] == "|"){
            std::vector<std::string> com;
            commands.push_back(com);
            cnt++;
            continue;
        }
        commands[cnt].push_back(vec[i]);
    }
    std::reverse(commands.begin(), commands.end());
}

void doPipe(std::vector< std::vector<std::string> > commands, int point, int pipe_fds[2]){
    if(point == commands.size() - 1){
        close(pipe_fds[0]);
        close(1);
        if (dup2(pipe_fds[1], 1) < 0) {
           perror("dup2 (child)");
           exit(1);
        }
        close(pipe_fds[1]);
        doCommand(commands[point]);
    }

    if(point != 0){
        close(pipe_fds[0]);
        close(1);
        if (dup2(pipe_fds[1], 1) < 0) {
           perror("dup2 (child)");
           exit(1);
        }
        close(pipe_fds[1]);
    }
    int new_pipe_fds[2];
    if (pipe(new_pipe_fds) < 0) {
        perror("pipe");
        return;
    }
    int child = fork();
    if (child < 0) {
        std::cout << "Failed fork\n";
        exit(1);
    }
    if (child == 0) { // 子プロセスの時
        doPipe(commands, point + 1, new_pipe_fds);
    }
    close(new_pipe_fds[1]);
    close(0);
    if (dup2(new_pipe_fds[0], 0) < 0) {
         perror("dup2 (parent)");
         exit(1);
    }
    close(new_pipe_fds[0]);
    int status;
    wait(&status);
    if (WEXITSTATUS(status) != 0) {
        std::cout << "Process "<< child << " existed with status(" << WEXITSTATUS(status) << ").\n";
    }
    doCommand(commands[point]);
}



void doCommand(std::vector<std::string> vec){
    char *commands[vec.size() + 1];
    int count = 0;
    for(int i = 0; i < vec.size(); i++){
        if(vec[i] == "<" && i != vec.size() - 1){
            int fd = open(vec[i + 1].c_str(), O_RDONLY|O_CREAT, 0755);
            if(fd < 0){
                return;
            }
            close(0);
            if (dup2(fd, 0) < 0) {
                perror("dup2error");
                close(fd);
                return;
            }
            close(fd);
            i++;
            continue;
        }
        else if(vec[i] == ">" && i != vec.size() - 1){
            int fd = open(vec[i + 1].c_str(), O_WRONLY|O_CREAT, 0755);
            if(fd < 0){
                return;
            }
            close(1);
            if (dup2(fd, 1) < 0) {
                perror("dup2error");
                close(fd);
                return;
            }
            close(fd);
            i++;
            continue;
        }
        commands[count] = (char *)vec[i].c_str();
        count++;
    }
    char *new_commands[count];
    for(int i = 0; i < count; i++){
        new_commands[i] = commands[i];
    }
    new_commands[count] = NULL;
    int status = execvp(new_commands[0], new_commands);
    perror(new_commands[0]);
    exit(status);
}



int ForkCommand(std::vector<std::string> vec){
    if (!vec.size()){
        std::cout <<  "コマンドが空です。" << "\n";
        return 0;
    }
    if(vec.size() == 1 && vec[0] == "bye"){
         std::cout << "\n";
         std::cout << "bye" << "\n";
         exit(0);
    }
    int child = fork();
    if (child < 0) {
        std::cout << "Failed fork\n";
        exit(1);
    }
    if (child == 0) { // 子プロセスの時
        doCommand(vec);
    }
    int status;
    wait(&status);
    if (WEXITSTATUS(status) != 0) {
        std::cout << "Process "<< child << " existed with status(" << WEXITSTATUS(status) << ").\n";
    }
    return WEXITSTATUS(status);
}

void ternaryOperator(std::vector<std::string> first_commands, std::vector<std::string> commands){
    std::vector<std::string> second_commands;
    std::vector<std::string> third_commands;
    int find = -1;
    for(int i = commands.size() - 1; i >= 0; i--){
        if(commands[i] == ":"){
            find = i;
            second_commands = std::vector<std::string>(commands.begin(), commands.begin() + i);
            break;
        }
        third_commands.push_back(commands[i]);
    }
    if(find < 0){
        std::cout << "error: not right ' : '\n";
        return;
    }
    std::reverse(third_commands.begin(), third_commands.end());
    if(ForkCommand(first_commands) == 0){
        Corpus(second_commands);
    } else {
        Corpus(third_commands);
    }
}

void Corpus(std::vector<std::string> vec){
    std::vector<std::string> commands;
    int vec_size = vec.size();
    for(int i = 0; i < vec_size; i++){
        if(vec[i] == ":"){
            std::cout << "error: not left ' ? '\n";
            return;
        }
        if(vec[i] == "?"){
            if(i == 0){
                std::cout << "error: center ' ? '\n";
                return;
            }
            ternaryOperator(commands, std::vector<std::string>(vec.begin() + i + 1, vec.end()));
            return;
        }
        if(vec[i] == "|"){
            std::vector< std::vector<std::string> > commands;
            splitPipe(vec, commands);
            int pipe[2];
            int child = fork();
            if (child < 0) {
                std::cout << "Failed fork\n";
                exit(1);
            }
            if (child == 0) { // 子プロセスの時
               doPipe(commands, 0, pipe);
            }
            int status;
            wait(&status);
            return;
        }
        commands.push_back(vec[i]);
    }
    ForkCommand(commands);
}
