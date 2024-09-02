#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>

struct cabecalho
{
    int offset_disponivel;
    int ultimoLido = 0;
    int ultimoRemovido = 0;
};

struct espaco_livre{
    int size;
    char marcador;
    int offset_prox;
};

struct inseridos
{
    char id_aluno[4];
    char sigla_disc[4];
    char nome_aluno[50];
    char nome_disc[50];
    float media;
    float freq;
} insere[6];

struct removidos
{
    char id_aluno[4];
    char sigla_disc[4];
} remove[4];

void removerRegistro(FILE *fd, struct cabecalho *header, char *id_aluno, char *sigla_disc)
{
    int reg_size;
    char buffer[1000];  // Buffer para armazenar o registro completo
    long offset = sizeof(struct cabecalho);  // Começar após o cabeçalho
    long reg_start_offset; // Guardará o inicio do registro que está sendo lido
    
    fseek(fd, offset, SEEK_SET);  // Navega até o início do arquivo após o cabeçalho

    //Percorre o arquivo em busca do registro correspondente ao id e sigla fornecidos
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
        if (strcmp(read_id_aluno, id_aluno) == 0 && strcmp(read_sigla_disc, sigla_disc) == 0){
            struct espaco_livre el;
            el.size = reg_size;
            el.marcador = '*';
            el.offset_prox = header->offset_disponivel;

            // Volta ao início do registro para sobrescrevê-lo
            fseek(fd, reg_start_offset, SEEK_SET);
            fwrite(&el, sizeof(el), 1, fd);

            // Atualiza o cabeçalho com o novo primeiro espaço livre, e atualiza o ultimo arquivo removido do vetor
            header->offset_disponivel = reg_start_offset;
            header->ultimoRemovido++;

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
        printf("Sigla Disciplina: %s\n", insere[i].sigla_disc);
        printf("Nome Aluno: %s\n", insere[i].nome_aluno);
        printf("Nome Disciplina: %s\n", insere[i].nome_disc);
        printf("Media: %.2f\n", insere[i].media);
        printf("Frequencia: %.2f\n\n", insere[i].freq);
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
    fread(remove, sizeof(remove), 1, fd);

    // Fecha o arquivo
    fclose(fd);

    // Exibe os registros lidos para verificar se tudo foi lido corretamente
    for (int i = 0; i < 4; i++)
    {
        printf("ID Aluno: %s\n", remove[i].id_aluno);
        printf("Sigla Disciplina: %s\n", remove[i].sigla_disc);
        printf("\n");
    }

    return 0;
}