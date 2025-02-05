#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h> 
#include <errno.h>

/*
    NOMES: Murilo de Miranda Silva - 812069
           Daniel de Souza Sobrinho Macedo - 813524
*/

// Variável global definida para o tamanho máximo da entrada de um comando
#define TAMANHO_MAXIMO_ENTRADA 10000

// Função que trata a captura do sinal do processo filho, impedindo-o de virar zumbi
void zombie(){
    waitpid(-1, NULL, WNOHANG);
}

// Função que implementa o pipe
void implementa_pipe(char *n1, char *n2) {

    pid_t ps;

    // Vetor de arquivos usado para o pipe
    int pipef[2];

    // Verifica se houve erro ao abrir o vetor de arquivos
    if (pipe(pipef) == -1) {
        perror("Erro na abertura do pipe\n");
        exit(EXIT_FAILURE);
    }

    // Processo 1
    ps = fork();

    // Verifica se houve erro na criação do processo 
    if (ps == -1) {
        perror("Erro na criação do processo 1\n");
        exit(EXIT_FAILURE);
    }
    // Processo filho 1, que redirecionará stdout
    if (ps == 0) { 
        close(pipef[0]); // Fecha a leitura do vetor de arquivos do processo filho 1
        dup2(pipef[1], STDOUT_FILENO); // Redireciona a saída padrão para o pipe
        close(pipef[1]); // Fecha a escrita do vetor de arquivos do processo filho 1

        // Executa o primeiro comando
        execlp(n1,n1,NULL); //execução do comando 
        perror("Erro na execução do primeiro comando"); // Em caso de erro
        exit(EXIT_FAILURE);
    }

    // Processo 2
    ps=fork();
    
    // Verifica se houve erro na criação do processo
    if (ps == -1) {
        perror("Erro na criação do processo 2\n");
        exit(EXIT_FAILURE);
    }
    // Processo filho 2, que redirecionará stdin
    if (ps == 0) { 
        close(pipef[1]); // Fecha a escrita do vetor de arquivos do processo filho 2
        dup2(pipef[0], STDIN_FILENO); // Redireciona a entrada padrão par a o pipe
        close(pipef[0]); // Fecha a leitura do vetor de arquivos do processo filho 2

        // Executa o segundo comando
        execlp(n2,n2,NULL); //execução do comando 
        perror("Erro na execução do segundo comando"); // Em caso de erro
        exit(EXIT_FAILURE);
    }

    // Fecha o vetor de arquivos do processo pai, já que não serão mais utilizadas
    close(pipef[0]);
    close(pipef[1]);

    // Aguarda os processos filhos terminarem
    waitpid(-1, NULL, 0);
    waitpid(-1, NULL, 0);
}


void redireciona_saida(char *comando, char *saida) {
    pid_t pid;
    int fd;

    // Abre ou cria o arquivo especificado para saída
    if ((fd = open(saida, O_WRONLY | O_CREAT | O_TRUNC, 0666)) == -1) {
        perror("Erro ao abrir o arquivo de saída");
        exit(EXIT_FAILURE);
    }

    // Cria um processo filho
    pid = fork();
    if (pid == -1) {
        perror("Erro na criação do processo filho");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // Filho: redireciona a saída padrão para o arquivo
        dup2(fd, STDOUT_FILENO);
        close(fd);  // Fecha o descritor de arquivo do arquivo de saída

        // Executa o comando
        execlp(comando, comando, NULL);

        // Se execlp retornar, houve um erro
        perror("Erro na execução do comando");
        exit(EXIT_FAILURE);
    } else {
        // Pai espera pelo filho
        wait(NULL);
    }
}

// Função responsável por executar o comando passado pelo usuário)
void executar_comando(char *comando, char **args, int background){

    // Criação do processo filho
    pid_t pid = fork();

    // Se o pid for menor que zero, significa que houve erro na criação
    if(pid<0){
        perror("Falha ao criar o processo");
        exit(EXIT_FAILURE);
    }

    // Se o pid for igual a zero, estamos tratando do processo filho 
    if(pid==0){

        // Execução do comando
        if(execvp(comando, args)==-1){
            perror("Falha na execução do processo filho");
            exit(EXIT_FAILURE);
        }
    } else{
        if(background==0){
            // Processo pai espera pelo processo filho (forground)
            wait(NULL);
        }
        else{
            //Processo pai fica em 'background' e recebe o sinal do processo filho quando este terminar
            signal(SIGCHLD, zombie);
        } 
    }
}

int main()
{
    // Vetor de entrada do usuário
    char entrada[TAMANHO_MAXIMO_ENTRADA];

    // Variáveis de controle do redirecionamento de E/S
    int pipeFlag = 0;
    int outFlag=0;

    // Ponteiro que aponta para os tokens
    char *token;

    // Vetor de ponteiros para os argumentos de cada comando
    char *args[80];

    // Indice que será usado para o vetor de argumentos
    int i;

    // Variável usada para definir o background
    int bg;
    
    printf("Bem-vindo ao Shell simplificado.\n");

    // Loop que se repete infintamente esperando a entrada de comandos feitos pelo usuário
    while(1){
        pipeFlag=0; //reseta a flag de comando com pipe
        outFlag=0; //reseta a flag de comando da saída de dados
        // Indica entrada do comando (prompt)
        printf("$ ");

        // Limpa o buffer para armazenar o prompt, garantindo a sua exibição imediata
        fflush(stdout);

        // Condição que perdura enquanto o usuário inserir comandos como entrada
        if(fgets(entrada, TAMANHO_MAXIMO_ENTRADA, stdin)==NULL){
            break;
        }

        //Retirada do "\n" das entradas
        entrada[strcspn(entrada, "\n")] = 0;

        // Verifica se a entrada é vazia (apenas Enter pressionado)
        if(strcmp(entrada, " ") == 0) {
            continue;
        }
        // Condição de saída, caso o usuário queira encerrar o shell com algum comando
        if(strcmp(entrada, "exit")==0){
            break;
        }

        //Uso do ponteiro 'token' atribuido à função strtok, usando a entrada do usuário e um espaço como parâmetros do token
        token = strtok(entrada, " ");

        // O indíce 0 do vetor será o comando passado pelo usuário (cd, ls, mkdir...)
        args[0] = token;

        // A partir daí, os outros argumentos passados passados para o vetor serão os parâmetros do comando (-l, -k, -s....)
        i = 0;

        //Loop condicionado à ser executado enquanto lhe forem passados comandos
        while(token !=NULL){
            i++;

            //Passa-se NULL como parâmetro, fazendo com que strtok() continue a partir de onde parou na última chamada. 
            token = strtok(NULL, " ");

            // Implementação do comando pipe  
            if (token != NULL && strcmp(token, "|") == 0) {
                pipeFlag=1;
                args[i] = NULL; // Termina a lista de argumentos para o comando atual
                token = strtok(NULL, " "); // Avança para o próximo comando após o '|'
                implementa_pipe(args[0], token); // Chama a função implementa_pipe com os comandos antes e depois do '|'
                break; 
            }
        
            // Implementação do comando de saída de dados 
            if (token != NULL && strcmp(token, ">") == 0) {
                outFlag=1;
                args[i] = NULL; // Termina a lista de argumentos para o comando atual
                token = strtok(NULL, " "); // Avança para o próximo comando após o '>'
                redireciona_saida(args[0], token); // Chama a função redireciona_saida com os comandos antes e depois do '>'
                break; 
            }
        
            // Preenche o vetor de argumentos com cada token passado pelo usuário
            args[i] = token;
        }    
        
        // Verificação se o comando está em background
        if(i>=2){
            if(strcmp(args[i-1], "&")==0){
                bg=1;
                args[i-1] = NULL; //Sinaliza o fim do comando 
            }
        }

        // Implementação do comando 'cd'
        if(strcmp(args[0], "cd")== 0){   

            // Retorna erro se houve falha na mudança de diretório
            if(chdir(args[1])!= 0){
                perror("Falha ao executar o comando cd");
            }
            else
                continue;
        }
        // Chamada da função para executar o comando passado pelo usuário
        if(pipeFlag==0 && outFlag==0){
            executar_comando(args[0], args, bg);
        }
    }
}