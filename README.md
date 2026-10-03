# bot_search

Trabalho Prático 1 de **Sistemas Operacionais I** (UFMA / DEINF), sobre API Unix e concorrência.

O programa conta quantas vezes um termo aparece em vários arquivos de texto usando o modelo **Bag-of-Tasks**: o processo pai (mestre) cria um processo filho (escravo) para cada arquivo com `fork()`, cada filho conta as ocorrências no seu arquivo e manda o resultado ao pai por um `pipe()` exclusivo, e o pai junta tudo depois de esperar os filhos com `wait()`.

## Como compilar

Precisa de Linux (ou outro sistema Unix-like), `gcc` e `make`.

```bash
make
```

## Como usar

```bash
./bot_search <termo_de_busca> <arquivo1.txt> <arquivo2.txt> ... <arquivoN.txt>
```

Exemplo com os arquivos da pasta `exemplos/` (o mesmo que `make run`):

```
$ ./bot_search "processo" exemplos/doc1.txt exemplos/doc2.txt exemplos/doc3.txt
[Mestre PID: 102] Criando 3 trabalhadores Bag-of-Tasks...
[Mestre] Resposta do Filho (PID 103 - 'exemplos/doc1.txt'): 3 ocorrências.
[Mestre] Resposta do Filho (PID 104 - 'exemplos/doc2.txt'): 3 ocorrências.
[Mestre] Resposta do Filho (PID 105 - 'exemplos/doc3.txt'): 5 ocorrências.
[Mestre] Todos os 3 processos filhos finalizaram.
--------------------------------------------------
TOTAL CONSOLIDADOS: 11 ocorrências do termo 'processo'.
```

## Como funciona

1. O pai valida os argumentos (precisa de um termo não vazio e pelo menos um arquivo).
2. Para cada arquivo, o pai cria um pipe e chama `fork()`.
3. O **filho** fecha a ponta de leitura do seu pipe e as pontas herdadas dos irmãos, lê o arquivo linha por linha com `getline()`, conta o termo com `strstr()`, escreve o total no pipe com `write()`, fecha o pipe e termina com `exit()`.
4. O **pai** fecha a ponta de escrita de cada pipe e guarda a de leitura, junto com o PID e o nome do arquivo.
5. O pai lê o resultado de cada filho com `read()`, fecha os pipes e chama `wait(NULL)` uma vez para cada filho.
6. Por fim, mostra o relatório por arquivo e o total.

Se um arquivo não puder ser aberto, o filho envia `-1` pelo pipe e o pai mostra uma mensagem de erro para aquele arquivo, sem somar nada ao total.

## Observações

- A busca é por **substring** e diferencia maiúsculas de minúsculas. Por isso, procurar `processo` também conta `processos`.
- Ocorrências sobrepostas não são contadas (em `aaaa`, o termo `aa` conta 2 vezes, não 3).

## Comandos do Makefile

| Comando      | O que faz                                      |
|--------------|------------------------------------------------|
| `make`       | Compila o programa                             |
| `make run`   | Compila e roda com os arquivos de `exemplos/`  |
| `make clean` | Apaga o executável                             |
| `make zip`   | Gera o `.zip` de entrega                       |

## Autores

Luana Lorhanni [@luanalorhanni](https://github.com/luanalorhanni) e Melissa Palhano