# Testes do bot_search

Testes feitos no Linux (Ubuntu no WSL). Os PIDs mudam a cada execução.

## 1. Execução básica

```bash
make run
```

```
[Mestre PID: 1095] Criando 3 trabalhadores Bag-of-Tasks...
[Mestre] Resposta do Filho (PID 1096 - 'exemplos/doc1.txt'): 3 ocorrências.
[Mestre] Resposta do Filho (PID 1097 - 'exemplos/doc2.txt'): 3 ocorrências.
[Mestre] Resposta do Filho (PID 1098 - 'exemplos/doc3.txt'): 5 ocorrências.
[Mestre] Todos os 3 processos filhos finalizaram.
--------------------------------------------------
TOTAL CONSOLIDADOS: 11 ocorrências do termo 'processo'.
```

O mestre criou um pipe e um filho (`fork()`) para cada arquivo. Cada filho contou as ocorrências e enviou o número pelo pipe. O mestre leu os pipes em ordem com `read()`, esperou os filhos com `wait()` e somou os resultados (3 + 3 + 5 = 11).

A contagem foi conferida com `grep -o "processo" <arquivo> | wc -l`, que dá os mesmos valores.

## 2. Tratamento de erros

| Comando | Resultado esperado |
|---|---|
| `./bot_search` | Mensagem de uso, código de saída 1 |
| `./bot_search "" exemplos/doc1.txt` | `Erro: o termo de busca não pode ser vazio.` |
| `./bot_search "processo" exemplos/doc1.txt naoexiste.txt` | `No such file or directory`; o filho reporta erro e o total soma só o arquivo válido (3) |
| `./bot_search "processo" bloqueado.txt` (após `chmod 000`) | `Permission denied` |
| `./bot_search "processo" exemplos/` | `Is a directory` |

Quando o filho não consegue ler o arquivo, ele envia `-1` pelo pipe. O mestre mostra o erro daquele arquivo e continua com os outros, já que as tarefas são independentes.

## 3. Casos de borda

| Comando | Resultado esperado | O que prova |
|---|---|---|
| `./bot_search "kernel" exemplos/*.txt` | Total 0 | Termo ausente retorna 0, não erro |
| `./bot_search "Processo" exemplos/*.txt` | Total 0 | A busca diferencia maiúsculas |
| Arquivo com 100.000 linhas contendo "processo" 2 vezes | 200000 ocorrências | Conta várias ocorrências por linha |
| 30 cópias do `doc1.txt` | 30 filhos, total 90 | Funciona para qualquer N |
| `./bot_search "processo" exemplos/*.txt > saida.txt` | Cabeçalho aparece só uma vez | O `fflush()` antes do `fork()` evita saída duplicada |

## 4. Descritores e memória

```bash
strace -f -e trace=pipe2,clone,close,read,write,wait4 ./bot_search "processo" exemplos/*.txt
valgrind --leak-check=full --track-fds=yes --trace-children=yes ./bot_search "processo" exemplos/*.txt
```

O `strace` mostra o mestre fechando a ponta de escrita, cada filho fechando a de leitura (e as herdadas dos irmãos) e um `wait4` por filho. O `valgrind` deve terminar com `no leaks are possible` e apenas os 3 descritores padrão abertos.