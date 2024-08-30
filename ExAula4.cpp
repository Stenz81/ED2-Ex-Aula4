#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>

int main () {
    FILE *fd;

    struct cabecalho{
        int offset;
        int ultimoLido = 0;
        int ultimoRemovido = 0;
    };

    struct inseridos{
        char id_aluno[4];
        char sigla_disc[4];
        char nome_aluno[50];
        char nome_disc[50];
        float media;
        float freq;
    }insere[6];

    struct removidos{
        char id_aluno[4];
        char sigla_disc[4];
    }remove[4];

    //Leitura dos registros a serem inseridos:

    // Abre o arquivo binário para leitura
    fd = fopen("insere.bin", "rb");
    if (fd == NULL) {
        printf("Nao foi possivel abrir o arquivo.\n");
        return 1;
    }

    // Le os registros do arquivo e armazena no vetor
    fread(insere, sizeof(insere), 1, fd);
    
    // Fecha o arquivo
    fclose(fd);

    // Exibe os registros lidos para verificar se tudo foi lido corretamente
    for (int i = 0; i < 6; i++) {
        printf("ID Aluno: %s\n", insere[i].id_aluno);
        printf("Sigla Disciplina: %s\n", insere[i].sigla_disc);
        printf("Nome Aluno: %s\n", insere[i].nome_aluno);
        printf("Nome Disciplina: %s\n", insere[i].nome_disc);
        printf("Media: %.2f\n", insere[i].media);
        printf("Frequencia: %.2f\n\n", insere[i].freq);
    }

    //Leitura dos registros a serem removidos:

     // Abre o arquivo binário para leitura
    fd = fopen("remove.bin", "rb");
    if (fd == NULL) {
        printf("Nao foi possivel abrir o arquivo.\n");
        return 1;
    }

    // Le os registros do arquivo e armazena no vetor
    fread(remove, sizeof(remove), 1, fd);
    
    // Fecha o arquivo
    fclose(fd);

    // Exibe os registros lidos para verificar se tudo foi lido corretamente
    for (int i = 0; i < 4; i++) {
        printf("ID Aluno: %s\n", remove[i].id_aluno);
        printf("Sigla Disciplina: %s\n", remove[i].sigla_disc);
        printf("\n");
    }

    return 0;

}