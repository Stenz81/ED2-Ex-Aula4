#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>

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

// Função para calcular o tamanho do registro
int calcularTamanhoRegistro(const Registro &reg)
{
    int tamanho_nome_aluno, tamanho_nome_disciplina;
    for (int i = 0; i < 50; i++)
    {
        if (reg.nome_aluno[i] == '\0')
        {          // Verifica se encontrou o espaço vazio (0x00)
            break; 
        }
        tamanho_nome_aluno++;
    }

    for (int i = 0; i < 50; i++)
    {
        if (reg.nome_disciplina[i] == '\0')
        {          // Verifica se encontrou o espaço vazio (0x00)
            break; 
        }
        tamanho_nome_disciplina++;
    }

    int tamanho = sizeof(reg.id_aluno) + 1 +
                  sizeof(reg.sigla_disciplina) + 1 +
                  tamanho_nome_aluno*sizeof(char) + 1 +              // +1 para o separador '#'
                  tamanho_nome_disciplina*sizeof(char) + 1 +
                  sizeof(reg.media) + 1 +
                  sizeof(reg.frequencia);
    return tamanho;
}

// Função para escrever um registro no arquivo
// Função para escrever um registro no arquivo
void inserirRegistro(FILE *file, const Registro &reg)
{
    int tamanho_registro = calcularTamanhoRegistro(reg);
    int tamanho_atual;

    // Verifica se existe espaço disponível (first-fit)
    bool espaco_encontrado = false;
    while (fread(&tamanho_atual, sizeof(int), 1, file) == 1)
    {
        if (tamanho_atual >= tamanho_registro)
        {
            // Move de volta para a posição inicial do espaço encontrado
            fseek(file, -sizeof(int), SEEK_CUR);

            // Escreve o novo tamanho do registro
            fwrite(&tamanho_registro, sizeof(int), 1, file);

            // Escreve o conteúdo do registro
            fwrite(reg.id_aluno, sizeof(reg.id_aluno), 1, file);
            fputc('#', file);
            fwrite(reg.sigla_disciplina, sizeof(reg.sigla_disciplina), 1, file);
            fputc('#', file);

            int count = 0;
            // Inserir nome_aluno caracter por caracter até encontrar 0x00
            for (int i = 0; i < 50; i++)
            {
                if (reg.nome_aluno[i] == '\0')
                {          // Verifica se encontrou o espaço vazio (0x00)
                    break; // Pula para o próximo campo
                }
                fputc(reg.nome_aluno[i], file); // Escreve o caractere no arquivo
            }

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

            espaco_encontrado = true;
            break;
        }
        else
        {
            // Avança para o próximo registro
            fseek(file, tamanho_atual, SEEK_CUR);
        }
    }

    if (!espaco_encontrado)
    {
        // Adiciona o registro no final do arquivo
        fseek(file, 0, SEEK_END); // Vai para o final do arquivo

        // Escreve o tamanho do registro no início
        fwrite(&tamanho_registro, sizeof(int), 1, file);

        // Escreve o conteúdo do registro
        fwrite(reg.id_aluno, sizeof(reg.id_aluno), 1, file);
        fputc('#', file);
        fwrite(reg.sigla_disciplina, sizeof(reg.sigla_disciplina), 1, file);
        fputc('#', file);
        fwrite(reg.nome_aluno, sizeof(char), sizeof(reg.nome_aluno), file);
        fputc('#', file);
        fwrite(reg.nome_disciplina, sizeof(char), sizeof(reg.nome_disciplina), file);
        fputc('#', file);
        fwrite(&reg.media, sizeof(float), 1, file);
        fputc('#', file);
        fwrite(&reg.frequencia, sizeof(float), 1, file);
    }

    fclose(file);
}
void removerRegistro(FILE *fd, struct cabecalho *header, char *id_aluno, char *sigla_disc)
{
    int reg_size;
    char buffer[1000];                      // Buffer para armazenar o registro completo
    long offset = sizeof(struct cabecalho); // Começar após o cabeçalho
    long reg_start_offset;                  // Guardará o inicio do registro que está sendo lido

    fseek(fd, offset, SEEK_SET); // Navega até o início do arquivo após o cabeçalho

    // Percorre o arquivo em busca do registro correspondente ao id e sigla fornecidos
    while (fread(&reg_size, sizeof(int), 1, fd))
    {
        // Guarda o início do registro atual
        long reg_start_offset = offset;

        // Lê o registro completo baseado no tamanho
        fread(buffer, reg_size, 1, fd);
        buffer[reg_size - sizeof(int)] = '\0';

        // Separa os campos do registro
        char *token = strtok(buffer, "#");
        char read_id_aluno[4], read_sigla_disc[4];
        strcpy(read_id_aluno, token);
        token = strtok(NULL, "#");
        strcpy(read_sigla_disc, token);

        // Verifica se o registro atual corresponde ao id e sigla fornecidos
        if (strcmp(read_id_aluno, id_aluno) == 0 && strcmp(read_sigla_disc, sigla_disc) == 0)
        {
            struct espaco_livre el;
            el.size = reg_size;
            el.marcador = '*';
            el.offset_prox = header->offset_disponivel;

            // Volta ao início do registro para sobrescrevê-lo
            fseek(fd, reg_start_offset, SEEK_SET);
            fwrite(&el, sizeof(el), 1, fd);

            // Atualiza o cabeçalho com o novo primeiro espaço livre, e atualiza o ultimo arquivo removido do vetor
            header->offset_disponivel = reg_start_offset;
            header->remover++;

            // Atualiza o cabeçalho no arquivo
            fseek(fd, 0, SEEK_SET);
            fwrite(header, sizeof(struct cabecalho), 1, fd);

            printf("Registro removido: ID Aluno %s, Disciplina %s\n", id_aluno, sigla_disc);
            return;
        }

        // Atualiza o offset para o próximo registro
        offset = ftell(fd);
    }

    printf("Registro não encontrado: ID Aluno %s, Disciplina %s\n", id_aluno, sigla_disc);
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

    inserirRegistro(fd, insere[0]);
    //inserirRegistro(fd, insere[1]);
    /*
    fseek(fd, 0, SEEK_SET);
    fwrite(&header, sizeof(struct cabecalho), 1, fd);

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