#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <vector>

struct cabecalho
{
    int offset_disponivel;
    int ler = 0;
    int remover = 0;
} header;

struct espaco_livre
{
    int size;
    char marcador;
    int offset_prox;
};

struct Registro
{
    char id_aluno[4];
    char sigla_disciplina[4];
    char nome_aluno[50];
    char nome_disciplina[50];
    float media;
    float frequencia;
} insere[6];

struct removidos
{
    char id_aluno[4];
    char sigla_disc[4];
} elimina[4];

//Função para verificar se um arquivo já existe
int arquivo_existe(const char *nome_arquivo) {
    FILE *arquivo = fopen(nome_arquivo, "rb");
    if (arquivo) {
        fclose(arquivo);
        return 1; // Arquivo existe
    }
    return 0; // Arquivo não existe
}

// Função para calcular o tamanho do registro
int calcularTamanhoRegistro(const Registro &reg)
{
    int tamanho_nome_aluno = 0, tamanho_nome_disciplina = 0;
    for (int i = 0; i < 50; i++)
    {

        if (reg.nome_aluno[i] == '\0')
        { // Verifica se encontrou o espaço vazio (0x00)
            break;
        }
        tamanho_nome_aluno++;
    }

    for (int i = 0; i < 50; i++)
    {
        if (reg.nome_disciplina[i] == '\0')
        { // Verifica se encontrou o espaço vazio (0x00)
            break;
        }
        tamanho_nome_disciplina++;
    }

    int tamanho = sizeof(reg.id_aluno) + 1 +
                  sizeof(reg.sigla_disciplina) + 1 +
                  tamanho_nome_aluno * sizeof(char) + 1 + // +1 para o separador '#'
                  tamanho_nome_disciplina * sizeof(char) + 1 +
                  sizeof(reg.media) + 1 +
                  sizeof(reg.frequencia);
    return tamanho;
}

// Função para escrever um registro no arquivo
void inserirRegistro(FILE *file, const Registro &reg)
{
    int tamanho_registro = calcularTamanhoRegistro(reg);
    int tamanho_atual;

    // bool espaco_encontrado = false;

    struct cabecalho header;
    fseek(file, 0, SEEK_SET);
    fread(&header, sizeof(struct cabecalho), 1, file);
    int offset_anterior = -1;
    int offset_atual = header.offset_disponivel;
    int offset_inserir = -1;

    struct espaco_livre esp_livre;

    // Verifica se existe espaço disponível (first-fit)
    while (offset_atual != -1)
    {
        fseek(file, offset_atual, SEEK_SET);
        fread(&esp_livre, sizeof(esp_livre), 1, file);

        if (esp_livre.size >= tamanho_registro)
        {
            offset_inserir = offset_atual;

            // Atualiza o encadeamento dos espaços livres
            if (offset_anterior == -1)
            { // Atualiza o cabeçalho se for o primeiro espaço livre
                header.offset_disponivel = esp_livre.offset_prox;
            }
            else
            {
                // Atualiza o próximo offset do registro anterior
                fseek(file, offset_anterior + offsetof(struct espaco_livre, offset_prox), SEEK_SET);
                fwrite(&esp_livre.offset_prox, sizeof(int), 1, file);
            }
            break;
        }

        // Atualiza os offsets para o próximo loop
        offset_anterior = offset_atual;
        offset_atual = esp_livre.offset_prox;
    }
    if (offset_inserir == -1)
    { // Se não encontrou um espaço adequado, insere no final do arquivo
        fseek(file, 0, SEEK_END);
        offset_inserir = ftell(file);
    }

    // Vai para o offset onde o registro será inserido e o escreve
    fseek(file, offset_inserir, SEEK_SET);

    // Escreve o novo tamanho do registro
    fwrite(&tamanho_registro, sizeof(int), 1, file);

    // Escreve o conteúdo do registro
    fwrite(reg.id_aluno, sizeof(reg.id_aluno), 1, file);
    fputc('#', file);
    fwrite(reg.sigla_disciplina, sizeof(reg.sigla_disciplina), 1, file);
    fputc('#', file);

    // Inserir nome_aluno caracter por caracter até encontrar 0x00
    for (int i = 0; i < 50; i++)
    {
        if (reg.nome_aluno[i] == '\0')
        {          // Verifica se encontrou o espaço vazio (0x00)
            break; // Pula para o próximo campo
        }
        fputc(reg.nome_aluno[i], file); // Escreve o caractere no arquivo
    }
    fputc('#', file);
    // Inserir nome_disciplina caracter por caracter até encontrar 0x00
    for (int i = 0; i < 50; i++)
    {
        if (reg.nome_disciplina[i] == '\0')
        {          // Verifica se encontrou o espaço vazio (0x00)
            break; // Pula para o próximo campo
        }
        fputc(reg.nome_disciplina[i], file); // Escreve o caractere no arquivo
    }
    fputc('#', file);
    fwrite(&reg.media, sizeof(float), 1, file);
    fputc('#', file);
    fwrite(&reg.frequencia, sizeof(float), 1, file);
    // Atualizar o cabeçalho no arquivo
    header.ler++;
    fseek(file, 0, SEEK_SET);
    fwrite(&header, sizeof(header), 1, file);
}

//Função para remover um registro
void removerRegistro(FILE *fd, const removidos rem)
{
    int reg_size;
    char buffer[1000];                      // Buffer para armazenar o registro completo
    long offset = sizeof(struct cabecalho); // Começar após o cabeçalho
    long reg_start_offset;                  // Guardará o inicio do registro que está sendo lido

    struct cabecalho header;
    fseek(fd, 0, SEEK_SET);
    fread(&header, sizeof(struct cabecalho), 1, fd);

    fseek(fd, offset, SEEK_SET); // Navega até o início do arquivo após o cabeçalho

    // Percorre o arquivo em busca do registro correspondente ao id e sigla fornecidos
    while (fread(&reg_size, sizeof(int), 1, fd))
    {

        reg_start_offset = ftell(fd) - sizeof(int); // Guarda o início do registro atual

        // Lê o registro completo baseado no tamanho
        fread(buffer, reg_size, 1, fd);
        buffer[reg_size] = '\0';

        // Leitura dos campos
        char read_id_aluno[4];
        char read_sigla_disciplina[4];
        char read_nome_aluno[50];
        char read_nome_disciplina[50];
        float read_media;
        float read_frequencia;

        // Separar os campos
        char *p = buffer;
        strncpy(read_id_aluno, p, 4);
        p += 5;
        strncpy(read_sigla_disciplina, p, 4);
        p += 5;
        strncpy(read_nome_aluno, p, 50);
        p += 51;
        strncpy(read_nome_disciplina, p, 50);
        p += 51;
        memcpy(&read_media, p, sizeof(float));
        p += sizeof(float);
        memcpy(&read_frequencia, p, sizeof(float));

        // Verifica se o registro atual corresponde ao id e sigla fornecidos
        if (strcmp(read_id_aluno, rem.id_aluno) == 0 && strcmp(read_sigla_disciplina, rem.sigla_disc) == 0)
        {
            struct espaco_livre el;
            el.size = reg_size;
            el.marcador = '*';
            el.offset_prox = header.offset_disponivel;

            // Volta ao início do registro para sobrescrevê-lo
            fseek(fd, reg_start_offset, SEEK_SET);
            fwrite(&el, sizeof(el), 1, fd);
            for(int i = 0; i < el.size-sizeof(el); i++){
                fputc('*', fd);
            }

            // Atualiza o cabeçalho com o novo primeiro espaço livre, e atualiza o ultimo arquivo removido do vetor
            header.offset_disponivel = reg_start_offset;
            header.remover++;

            // Atualiza o cabeçalho no arquivo
            fseek(fd, 0, SEEK_SET);
            fwrite(&header, sizeof(struct cabecalho), 1, fd);

            printf("Registro removido: ID Aluno %s, Disciplina %s\n", rem.id_aluno, rem.sigla_disc);
            return;
        }

        // Atualiza o offset para o próximo registro
        offset = ftell(fd);
    }

    printf("Registro nao encontrado: ID Aluno %s, Disciplina %s\n", rem.id_aluno, rem.sigla_disc);
}

void compactar (FILE *fd){
    int posicao_atual, fim_atual, inicio_proximo;
    int reg_size_atual, reg_size_proximo;
    char verificador, buffer;

    fseek(fd, sizeof(struct cabecalho), SEEK_SET);

    while (fread(&reg_size_atual, sizeof(int), 1, fd)){
        posicao_atual = ftell(fd) - sizeof(int); //grava o inicio do registro atual

        fread(&verificador, sizeof(char), 1, fd);
        if(verificador == '*'){ //verifica se o espaço está livre 
            fseek(fd, posicao_atual + reg_size_atual, SEEK_SET);//busca pelo primeiro registro depois do espaco livre
            inicio_proximo = ftell(fd);//grava o inicio do proximo registro
            fread(&reg_size_proximo, sizeof(int), 1, fd);//grava o tamanho do proximo registro


            for (int i = 0; i < reg_size_proximo + sizeof(int); i++){
                fseek(fd, inicio_proximo + i, SEEK_SET);
                fread(&buffer, sizeof(char), 1, fd);
                fseek(fd, posicao_atual, SEEK_SET);
                fwrite(&buffer, sizeof(char), 1, fd);
                posicao_atual++;
            }

            fim_atual = posicao_atual;
            fseek(fd, inicio_proximo + reg_size_proximo, SEEK_SET);


        }

    }


}

int main()
{
    FILE *fd;

    // Leitura dos registros a serem inseridos:

    // Abre o arquivo binário para leitura
    fd = fopen("insere.bin", "rb");
    if (fd == NULL)
    {
        printf("Nao foi possivel abrir o arquivo.\n");
        return 1;
    }

    // Le os registros do arquivo e armazena no vetor
    fread(insere, sizeof(insere), 1, fd);

    // Fecha o arquivo
    fclose(fd);

    // Exibe os registros lidos para verificar se tudo foi lido corretamente
    for (int i = 0; i < 6; i++)
    {
        printf("ID Aluno: %s\n", insere[i].id_aluno);
        printf("Sigla Disciplina: %s\n", insere[i].sigla_disciplina);
        printf("Nome Aluno: %s\n", insere[i].nome_aluno);
        printf("Nome Disciplina: %s\n", insere[i].nome_disciplina);
        printf("Media: %.2f\n", insere[i].media);
        printf("Frequencia: %.2f\n\n", insere[i].frequencia);
    }

    // Leitura dos registros a serem removidos:

    // Abre o arquivo binário para leitura
    fd = fopen("remove.bin", "rb");
    if (fd == NULL)
    {
        printf("Nao foi possivel abrir o arquivo.\n");
        return 1;
    }

    // Le os registros do arquivo e armazena no vetor
    fread(elimina, sizeof(elimina), 1, fd);

    // Fecha o arquivo
    fclose(fd);

    // Exibe os registros lidos para verificar se tudo foi lido corretamente
    for (int i = 0; i < 4; i++)
    {
        printf("ID Aluno: %s\n", elimina[i].id_aluno);
        printf("Sigla Disciplina: %s\n", elimina[i].sigla_disc);
        printf("\n");
    }

    header.offset_disponivel = -1;
    header.ler = 0;
    header.remover = 0;

    fd = fopen("listaRegistros.bin", "wb+");
    if (fd == NULL)
    {
        printf("Nao foi possivel abrir o arquivo.\n");
        return 1;
    }

    fseek(fd, 0, SEEK_SET);
    fwrite(&header, sizeof(struct cabecalho), 1, fd);

    inserirRegistro(fd, insere[0]);
    inserirRegistro(fd, insere[1]);
    inserirRegistro(fd, insere[2]);
    inserirRegistro(fd, insere[3]);
    removerRegistro(fd, elimina[0]);
    inserirRegistro(fd, insere[4]);
    inserirRegistro(fd, insere[2]);
    fclose(fd);

    /*
    int choice;
    do{
        printf("Selecione uma das seguintes opções:\n0-Sair;\n1-Inserir novo registro;\n2-Remover registro;\n3-Realizar a compactação do arquivo;\nOpção:");
        scanf("%i", &choice);
        switch (choice)
        {
        case 1:

            break;

        case 2:
            break;

        case 3:
            break;

        }

    }while(choice != 0);
*/
}