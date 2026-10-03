#define _POSIX_C_SOURCE 200809L /* necessário para o getline() */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

/* Valor que o filho envia quando não consegue abrir/ler o arquivo */
#define ERRO_ARQUIVO -1

/* Guarda as informações de cada filho que o pai precisa para o relatório */
typedef struct {
    pid_t pid;           /* PID do filho (retorno do fork no pai) */
    int fd_leitura;      /* ponta de leitura do pipe deste filho */
    const char *arquivo; /* nome do arquivo que o filho processa */
} Trabalhador;

/*
 * Conta quantas vezes 'termo' aparece em 'linha'.
 * O strstr() só acha a primeira ocorrência, então repetimos a busca
 * a partir da posição logo depois de cada ocorrência encontrada.
 */
static int conta_na_linha(const char *linha, const char *termo)
{
    int total = 0;
    size_t tam_termo = strlen(termo);
    const char *pos = strstr(linha, termo);

    while (pos != NULL) {
        total++;
        pos = strstr(pos + tam_termo, termo);
    }
    return total;
}

/*
 * Abre o arquivo, lê linha por linha e soma as ocorrências do termo.
 * Retorna o total encontrado ou ERRO_ARQUIVO se algo der errado.
 */
static int conta_no_arquivo(const char *nome_arquivo, const char *termo)
{
    FILE *fp = fopen(nome_arquivo, "r");
    if (fp == NULL) {
        perror(nome_arquivo);
        return ERRO_ARQUIVO;
    }

    char *linha = NULL; /* o getline() aloca e aumenta o buffer sozinho */
    size_t capacidade = 0;
    int total = 0;

    while (getline(&linha, &capacidade, fp) != -1) {
        total += conta_na_linha(linha, termo);
    }

    /* getline() também retorna -1 em erro de leitura, não só no fim */
    if (ferror(fp)) {
        perror(nome_arquivo);
        total = ERRO_ARQUIVO;
    }

    free(linha);
    fclose(fp);
    return total;
}

/*
 * Código executado pelo filho. Nunca retorna: termina com exit().
 * 'trab' e 'qtd_anteriores' servem para fechar os pipes dos irmãos
 * criados antes, que o filho herdou do pai no fork().
 */
static void executa_filho(int pipefd[2], const char *arquivo, const char *termo,
                          const Trabalhador *trab, int qtd_anteriores)
{
    /* O filho só escreve, então fecha a ponta de leitura do seu pipe */
    close(pipefd[0]);

    /* Fecha as pontas de leitura dos irmãos que foram herdadas */
    for (int i = 0; i < qtd_anteriores; i++) {
        close(trab[i].fd_leitura);
    }

    int ocorrencias = conta_no_arquivo(arquivo, termo);

    /* Envia o resultado ao pai como um int "cru" */
    ssize_t escritos = write(pipefd[1], &ocorrencias, sizeof(ocorrencias));
    if (escritos != (ssize_t) sizeof(ocorrencias)) {
        perror("write");
        close(pipefd[1]);
        exit(EXIT_FAILURE);
    }

    close(pipefd[1]);
    exit(ocorrencias == ERRO_ARQUIVO ? EXIT_FAILURE : EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
    /* Validação dos argumentos: precisa do termo e de pelo menos 1 arquivo */
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <termo_de_busca> <arquivo1.txt> ... <arquivoN.txt>\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    const char *termo = argv[1];
    if (termo[0] == '\0') {
        /* termo vazio faria o strstr() achar "ocorrências" infinitas */
        fprintf(stderr, "Erro: o termo de busca não pode ser vazio.\n");
        return EXIT_FAILURE;
    }

    int n = argc - 2; /* quantidade de arquivos = quantidade de filhos */

    Trabalhador *trab = malloc(n * sizeof(Trabalhador));
    if (trab == NULL) {
        perror("malloc");
        return EXIT_FAILURE;
    }

    printf("[Mestre PID: %d] Criando %d trabalhadores Bag-of-Tasks...\n",
           (int) getpid(), n);

    int criados = 0; /* quantos filhos foram realmente criados */

    for (int i = 0; i < n; i++) {
        const char *arquivo = argv[i + 2];
        int pipefd[2];

        if (pipe(pipefd) < 0) {
            perror("pipe");
            break; /* para de criar, mas cuida dos filhos que já existem */
        }

        /*
         * Esvazia o buffer do printf antes do fork. Sem isso, se a saída
         * for redirecionada para um arquivo, o texto pendente seria copiado
         * para o filho e apareceria repetido.
         */
        fflush(stdout);

        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            close(pipefd[0]);
            close(pipefd[1]);
            break;
        }

        if (pid == 0) {
            /* ===== PROCESSO FILHO ===== */
            executa_filho(pipefd, arquivo, termo, trab, criados);
        }

        /* ===== PROCESSO PAI ===== */
        close(pipefd[1]); /* o pai só lê, então fecha a ponta de escrita */

        trab[criados].pid = pid;
        trab[criados].fd_leitura = pipefd[0];
        trab[criados].arquivo = arquivo;
        criados++;
    }

    /* Lê a resposta de cada filho pelo seu pipe */
    long total = 0;

    for (int i = 0; i < criados; i++) {
        int ocorrencias;
        ssize_t lidos = read(trab[i].fd_leitura, &ocorrencias, sizeof(ocorrencias));

        if (lidos != (ssize_t) sizeof(ocorrencias)) {
            /* lidos == 0 significa que o filho fechou o pipe sem escrever */
            fprintf(stderr, "[Mestre] Filho (PID %d - '%s') não enviou resposta válida.\n",
                    (int) trab[i].pid, trab[i].arquivo);
        } else if (ocorrencias == ERRO_ARQUIVO) {
            printf("[Mestre] Resposta do Filho (PID %d - '%s'): erro ao ler o arquivo.\n",
                   (int) trab[i].pid, trab[i].arquivo);
        } else {
            printf("[Mestre] Resposta do Filho (PID %d - '%s'): %d ocorrências.\n",
                   (int) trab[i].pid, trab[i].arquivo, ocorrencias);
            total += ocorrencias;
        }

        close(trab[i].fd_leitura);
    }

    /* Espera todos os filhos terminarem (evita processos zumbis) */
    for (int i = 0; i < criados; i++) {
        if (wait(NULL) < 0) {
            perror("wait");
        }
    }

    printf("[Mestre] Todos os %d processos filhos finalizaram.\n", criados);
    printf("--------------------------------------------------\n");
    printf("TOTAL CONSOLIDADOS: %ld ocorrências do termo '%s'.\n", total, termo);

    free(trab);

    /* Se nem todos os filhos foram criados, sinaliza erro na saída */
    return (criados == n) ? EXIT_SUCCESS : EXIT_FAILURE;
}