#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#define MAX_INSERE 6
#define MAX_REMOVE 4

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <vector>

#define _POSIX1_SOURCE 2
#include <unistd.h>

int ftruncate(int fildes, off_t length);

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
} insere[MAX_INSERE];

struct removidos
{
    char id_aluno[4];
    char sigla_disc[4];
} elimina[MAX_REMOVE];

// Função para verificar se um arquivo já existe
int arquivo_existe(const char *nome_arquivo)
{
    FILE *arquivo = fopen(nome_arquivo, "rb");
    if (arquivo)
    {
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
    fwrite(&header, sizeof(struct cabecalho), 1, file);

    fflush(file);
}


// Função para remover um registro
void removerRegistro(FILE *fd, const removidos rem) {
    int reg_size;
    char buffer[1000];                      // Buffer para armazenar o registro completo
    long offset = sizeof(struct cabecalho); // Começar após o cabeçalho
    long reg_start_offset;                  // Guardará o início do registro que está sendo lido

    struct cabecalho header;
    fseek(fd, 0, SEEK_SET);
    fread(&header, sizeof(struct cabecalho), 1, fd);

    fseek(fd, offset, SEEK_SET); // Navega até o início do arquivo após o cabeçalho

    // Percorre o arquivo em busca do registro correspondente ao id e sigla fornecidos
    while (fread(&reg_size, sizeof(int), 1, fd)) {
        reg_start_offset = ftell(fd) - sizeof(int); // Guarda o início do registro atual

        // Lê o registro completo baseado no tamanho
        fread(buffer, reg_size, 1, fd);

        // Leitura dos campos
        char read_id_aluno[4] = {0};
        char read_sigla_disciplina[4] = {0};

        // Separar os campos
        char *p = buffer;
        strncpy(read_id_aluno, p, 4);
        p += 4; // Avança pelo tamanho do ID do aluno
        p++; // Pular o caractere separador '#'
        strncpy(read_sigla_disciplina, p, 4);
        p += 4; // Avança pelo tamanho da sigla da disciplina
        p++; // Pular o caractere separador '#'

        // Verifica se o registro atual corresponde ao id e sigla fornecidos
        if (strcmp(read_id_aluno, rem.id_aluno) == 0 && strcmp(read_sigla_disciplina, rem.sigla_disc) == 0) {
            struct espaco_livre el;
            el.size = reg_size;
            el.marcador = '*';
            el.offset_prox = header.offset_disponivel;

            // Volta ao início do registro para sobrescrevê-lo
            fseek(fd, reg_start_offset, SEEK_SET);
            fwrite(&el, sizeof(struct espaco_livre), 1, fd);

            // Preenche o restante do espaço livre com '*'
            fseek(fd, reg_start_offset + sizeof(struct espaco_livre), SEEK_SET);
            for (int i = 0; i < reg_size - sizeof(struct espaco_livre); i++) {
                fputc('*', fd);
            }

            // Atualiza o cabeçalho com o novo primeiro espaço livre
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
        fseek(fd, offset, SEEK_SET); // Necessário para garantir que o arquivo seja lido corretamente
    }

    printf("Registro não encontrado: ID Aluno %s, Disciplina %s\n", rem.id_aluno, rem.sigla_disc);
}

void compactar(FILE *fd) {
    int posicao_atual, fim_atual, inicio_proximo;
    int reg_size_atual, reg_size_proximo;
    char verificador, buffer;

    // Vai para o início do arquivo após o cabeçalho
    fseek(fd, sizeof(struct cabecalho), SEEK_SET);

    // Pega o tamanho total do arquivo
    fseek(fd, 0, SEEK_END);
    long tamanho_arquivo = ftell(fd);

    // Volta para o início do primeiro registro
    fseek(fd, sizeof(struct cabecalho), SEEK_SET);

    // Compactar os registros
    while (fread(&reg_size_atual, sizeof(int), 1, fd)) {
        posicao_atual = ftell(fd) - sizeof(int); // Guarda o início do registro atual

        // Verifica se o registro está marcado como removido (espaço livre)
        fread(&verificador, sizeof(char), 1, fd);
        if (verificador == '*') {
            // Busca o próximo registro depois do espaço livre
            fseek(fd, posicao_atual + reg_size_atual, SEEK_SET);

            // Verifica se chegou ao final do arquivo
            if (ftell(fd) >= tamanho_arquivo) {
                // Se o espaço livre está no final do arquivo, truncar o arquivo
                printf("Removendo espaco livre no final do arquivo.\n");
                ftruncate(fileno(fd), posicao_atual);
                break;
            }

            // Pega o início do próximo registro e seu tamanho
            inicio_proximo = ftell(fd);
            fread(&reg_size_proximo, sizeof(int), 1, fd);

            // Move o próximo registro para ocupar o espaço livre
            for (int i = 0; i < reg_size_proximo + sizeof(int); i++) {
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
/*
#include <stdio.h>
#include <unistd.h> // Para usar ftruncate

void compactar(FILE *fd)
{
    int posicao_atual, reg_size_atual, reg_size_proximo;
    char verificador;
    long fim_arquivo;
    char buffer[1000];  // Buffer temporário para mover registros

    // Move o ponteiro para o final do arquivo para determinar seu tamanho
    fseek(fd, 0, SEEK_END);
    fim_arquivo = ftell(fd);

    // Volta para o início dos registros (após o cabeçalho)
    fseek(fd, sizeof(struct cabecalho), SEEK_SET);

    while (ftell(fd) < fim_arquivo && fread(&reg_size_atual, sizeof(int), 1, fd)) {
        posicao_atual = ftell(fd) - sizeof(int); // Guarda o início do registro atual

        // Lê o primeiro byte do registro para verificar se é um espaço livre
        fread(&verificador, sizeof(char), 1, fd);
        if (verificador == '*') {
            // Registro removido (espaço livre)

            // Move o ponteiro para o próximo registro após o espaço livre
            fseek(fd, posicao_atual + reg_size_atual, SEEK_SET);

            // Verifica se já estamos no final do arquivo
            long inicio_proximo = ftell(fd);
            if (inicio_proximo >= fim_arquivo) {
                // Não há mais registros após este espaço livre, truncar o arquivo
                printf("Espaço livre encontrado no final do arquivo. Truncando...\n");
                ftruncate(fileno(fd), posicao_atual); // Remove o espaço livre ao truncar o arquivo
                break;
            }

            // Lê o próximo registro
            fread(&reg_size_proximo, sizeof(int), 1, fd); // Tamanho do próximo registro

            // Lê o conteúdo do próximo registro no buffer
            fseek(fd, inicio_proximo + sizeof(int), SEEK_SET);
            fread(buffer, reg_size_proximo, 1, fd);

            // Volta ao início do espaço livre para sobrescrevê-lo com o próximo registro
            fseek(fd, posicao_atual, SEEK_SET);
            fwrite(&reg_size_proximo, sizeof(int), 1, fd);  // Escreve o tamanho do registro
            fwrite(buffer, reg_size_proximo, 1, fd);        // Escreve o conteúdo do registro

            // Move os registros subsequentes
            long prox_inicio = inicio_proximo + sizeof(int) + reg_size_proximo;
            fseek(fd, prox_inicio, SEEK_SET);

            // Atualiza fim_arquivo para refletir que o arquivo pode ter sido reduzido
            fim_arquivo -= (inicio_proximo - posicao_atual); // Atualiza o tamanho do arquivo após a compactação

        } else {
            // Registro válido, apenas mova o ponteiro para o próximo registro
            fseek(fd, posicao_atual + reg_size_atual, SEEK_SET);
        }
    }*/

    // Truncar o arquivo caso o último espaço livre não tenha registros subsequentes
    fseek(fd, 0, SEEK_END);
    fim_arquivo = ftell(fd);
    ftruncate(fileno(fd), fim_arquivo);
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

    if (arquivo_existe("listaRegistros.bin"))//se o arquivo já existe, abbre o arquivo
    {
        fd = fopen("listaRegistros.bin", "rb+");
        if (fd == NULL)
        {
            printf("Nao foi possivel abrir o arquivo.\n");
            return 1;
        }
    }
    else //se nao existe, cria e inicia o cabeçalho
    {
        fd = fopen("listaRegistros.bin", "wb+");
        if (fd == NULL)
        {
            printf("Nao foi possivel abrir o arquivo.\n");
            return 1;
        }
        header.offset_disponivel = -1;
        header.ler = 0;
        header.remover = 0;

        fseek(fd, 0, SEEK_SET);
        fwrite(&header, sizeof(struct cabecalho), 1, fd);
    }

    int choice;
    struct cabecalho hdr;
    do
    {
        printf("\nSelecione uma das seguintes opcoes:\n0-Sair;\n1-Inserir novo registro;\n2-Remover registro;\n3-Realizar a compactacao do arquivo;\nOpcao:");
        scanf("%i", &choice);
        switch (choice)
        {

        case 0:
            fseek(fd, 0, SEEK_SET);
            fflush(fd);
            fclose(fd);
            break;

        case 1:

            fseek(fd, 0, SEEK_SET);
            fread(&hdr, sizeof(struct cabecalho), 1, fd);
            if (hdr.ler < MAX_INSERE)
            {
                inserirRegistro(fd, insere[hdr.ler]);
            }else{
                printf("Todos os registros já foram inseridos. ");
            }            
            break;

        case 2:
            fseek(fd, 0, SEEK_SET);
            fread(&hdr, sizeof(struct cabecalho), 1, fd);
            if (hdr.remover < MAX_REMOVE)
            {
                removerRegistro(fd, elimina[hdr.remover]);
            }else{
                printf("Todos os registros da lista já foram removidos. ");
            }
            break;

        case 3:
            compactar(fd);
            break;
        
        default:
            printf("Insira uma opcao valida.\n");
            break;
        }

    } while (choice != 0);
    return 0;
}