# Makefile - bot_search

CC      = gcc
CFLAGS  = -Wall -Wextra -pedantic -std=c11 -g
ALVO    = bot_search
FONTE   = bot_search.c

.PHONY: all run clean zip

all: $(ALVO)

$(ALVO): $(FONTE)
	$(CC) $(CFLAGS) -o $(ALVO) $(FONTE)

# Executa um teste rápido com os arquivos de exemplo
run: $(ALVO)
	./$(ALVO) "processo" exemplos/doc1.txt exemplos/doc2.txt exemplos/doc3.txt

clean:
	rm -f $(ALVO) *.o

# Gera o pacote de entrega nesta pasta.
zip: clean
	rm -f bot_search.zip
	zip -r bot_search.zip bot_search.c Makefile exemplos relatorio.pdf
	zip -j bot_search.zip ../README.md