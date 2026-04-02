# 🚀 AXIS Language

![AXIS Logo](EXTENSION/axis-extension/logo.png)

> **"Só fiz por diversão, não é pra ser nada grande nem profissional"**

A **AXIS** é uma linguagem de baixo nível criada para quem quer sentir o **poder total** sobre a memória. Chega de abstrações chatas, aqui é você e os ponteiros!

## 🚀 Como Rodar a AXIS

Siga os passos abaixo para compilar o interpretador e rodar seu primeiro código:

DOCUMENTAÇÃO OFICIAL

**AXIS**

**LINGUAGEM DE PROGRAMAÇÃO**

Documentação de Referência - v2.0

Baseada em fita infinita de 30.000 células (0-255). Cada célula armazena um número inteiro sem sinal de 8 bits. O ponteiro indica a célula ativa. Todos os comandos operam sobre a célula ativa ou navegam pela fita.

# **1\. Visão Geral**

AXIS é uma linguagem interpretada de baixo nível, inspirada no modelo de fita do Brainfuck, com sintaxe em português e suporte a variáveis nomeadas, rótulos, entrada/saída e controle de fluxo.

## **Modelo de memória**

A fita possui 30.000 células numeradas de 0 a 29999. Cada célula guarda um valor de 0 a 255 (8 bits, com overflow circular). O ponteiro aponta para a célula ativa.

## **Estrutura de um programa**

    
    \$ Isso é um comentário (tudo após \$ é ignorado)

    NOMEAR 0 minha_var \$ Define apelido para posição 0

    ROTULO inicio \$ Marca um ponto de salto

    MAIS \$ Incrementa a célula ativa

    SE_IGUAL 10 fim \$ Pula para 'fim' se valor == 10

    PULAR inicio \$ Volta ao início

    ROTULO fim

    VALOR \$ Imprime o número atual

    SAIR

## **Compilar e executar**

    \$ Compilar

    make

    \$ Executar

    ./axis meu_programa.axis

    \$ Executar em modo debug (verbose)

    ./axis meu_programa.axis -v

# **2\. Referência de Comandos**

Comandos marcados com **★** são adições da versão 2.0.

## **2.1 Aritmética**

| **Comando**            | **Argumentos**     | **Descrição**                                                     |
| ---------------------- | ------------------ | ----------------------------------------------------------------- |
| **MAIS**               | -                  | Incrementa em 1 o valor da célula ativa (+1). Overflow: 255 → 0.  |
| **MENOS**              | -                  | Decrementa em 1 o valor da célula ativa (−1). Underflow: 0 → 255. |
| **ZERAR**              | -                  | Define o valor da célula ativa como 0.                            |
| **SOMA &lt;n&gt;**     | &lt;n&gt;: inteiro | Soma o valor inteiro n ao valor da célula ativa.                  |
| **SOMAR &lt;n&gt;**    | &lt;n&gt;: inteiro | Alias de SOMA. Comportamento idêntico.                            |
| **SUBTRAIR &lt;n&gt;** | &lt;n&gt;: inteiro | Subtrai o valor inteiro n do valor da célula ativa.               |
| **SUBTRAI &lt;n&gt;**  | &lt;n&gt;: inteiro | Alias de SUBTRAIR. Comportamento idêntico.                        |

## **2.2 Movimentação na fita**

| **Comando**                            | **Argumentos**                           | **Descrição**                                                                       |
| -------------------------------------- | ---------------------------------------- | ----------------------------------------------------------------------------------- |
| **DIREITA**                            | -                                        | Move o ponteiro uma posição para a direita. Avisa no limite.                        |
| **ESQUERDA**                           | -                                        | Move o ponteiro uma posição para a esquerda. Avisa no limite.                       |
| **IR &lt;pos\|nome&gt;**               | &lt;pos&gt;: 0-29999 ou nome de variável | Move o ponteiro para a posição numérica ou para o endereço de uma variável nomeada. |
| **COPIAR &lt;orig&gt; &lt;dest&gt; ★** | &lt;orig&gt;, &lt;dest&gt;: 0-29999      | Copia o valor da posição orig para a posição dest sem mover o ponteiro.             |

## **2.3 Variáveis (agenda)**

Variáveis são apelidos para posições fixas da fita. Elas não alocam memória - apenas nomeiam um endereço.

| **Comando**                         | **Argumentos**                                   | **Descrição**                                                           |
| ----------------------------------- | ------------------------------------------------ | ----------------------------------------------------------------------- |
| **NOMEAR &lt;pos&gt; &lt;nome&gt;** | &lt;pos&gt;: 0-29999 &lt;nome&gt;: identificador | Registra o nome como apelido para a posição pos. Limite: 512 variáveis. |

    NOMEAR 0 x \$ posição 0 agora se chama 'x'

    NOMEAR 1 y \$ posição 1 agora se chama 'y'

    IR x \$ ponteiro vai para posição 0

    SOMA 42 \$ x = 42

    COPIAR 0 1 \$ y = x (copia valor de pos 0 para pos 1)

## **2.4 Controle de fluxo**

| **Comando**                          | **Argumentos**                       | **Descrição**                                                                          |
| ------------------------------------ | ------------------------------------ | -------------------------------------------------------------------------------------- |
| **ROTULO &lt;nome&gt;**              | &lt;nome&gt;: identificador          | Declara um ponto de salto. Deve estar em sua própria linha. Mapeado antes da execução. |
| **PULAR &lt;nome&gt;**               | &lt;nome&gt;: rótulo                 | Salta incondicionalmente para o rótulo especificado.                                   |
| **SE_IGUAL &lt;n&gt; &lt;rot&gt;**   | &lt;n&gt;: 0-255 &lt;rot&gt;: rótulo | Salta para rot se o valor da célula ativa for igual a n.                               |
| **SE_MENOR &lt;n&gt; &lt;rot&gt; ★** | &lt;n&gt;: 0-255 &lt;rot&gt;: rótulo | Salta para rot se o valor da célula ativa for menor que n.                             |
| **SE_MAIOR &lt;n&gt; &lt;rot&gt; ★** | &lt;n&gt;: 0-255 &lt;rot&gt;: rótulo | Salta para rot se o valor da célula ativa for maior que n.                             |

**Exemplo - loop de contagem**

    ZERAR \$ célula 0 = 0

    ROTULO loop

    SE_IGUAL 10 fim \$ se == 10, para

    MAIS \$ incrementa

    PULAR loop

    ROTULO fim

    VALOR \$ imprime 10

**Exemplo - comparação com SE_MENOR / SE_MAIOR**

    SOMA 42

    SE_MENOR 50 eh_menor \$ 42 < 50 → pula

    PULAR fim

    ROTULO eh_menor

    FALAR "Menor que 50"

    ROTULO fim

    SAIR

## **2.5 Entrada e Saída**

| **Comando**       | **Argumentos**          | **Descrição**                                                                 |
| ----------------- | ----------------------- | ----------------------------------------------------------------------------- |
| **ENTRADA**       | -                       | Lê um inteiro (0-255) do teclado e armazena na célula ativa.                  |
| **VALOR**         | -                       | Imprime o valor numérico da célula ativa seguido de nova linha.               |
| **FALAR "texto"** | "texto": string literal | Imprime o texto entre aspas seguido de nova linha.                            |
| **FALAR**         | -                       | Sem aspas: imprime o caractere ASCII correspondente ao valor da célula ativa. |

    \$ Imprime 'A' (ASCII 65)

    SOMA 65

    FALAR

    \$ Imprime texto fixo

    FALAR "Olá, mundo!"

    \$ Lê número e dobra

    ENTRADA

    SOMA 0 \$ valor dobrado: use REPETE 2 \[MAIS\] ou SOMA direto

## **2.6 Repetição inline**

O comando REPETE executa um bloco de comandos N vezes na mesma linha, usando colchetes como delimitadores.

| **Comando**                   | **Argumentos**                               | **Descrição**                                           |
| ----------------------------- | -------------------------------------------- | ------------------------------------------------------- |
| **REPETE &lt;n&gt; \[cmds\]** | &lt;n&gt;: inteiro \[cmds\]: comandos inline | Executa os comandos dentro de \[ \] exatamente n vezes. |

    REPETE 5 \[MAIS\] \$ célula += 5

    REPETE 3 \[DIREITA MAIS\] \$ avança 3 células e incrementa cada uma

    REPETE 65 \[MAIS\] FALAR \$ imprime 'A' (ASCII 65)

## **2.7 Módulos externos**

| **Comando**                  | **Argumentos**                            | **Descrição**                                                                                       |
| ---------------------------- | ----------------------------------------- | --------------------------------------------------------------------------------------------------- |
| **CARREGAR &lt;arquivo&gt;** | &lt;arquivo&gt;: caminho do arquivo .axis | Interpreta o arquivo indicado linha a linha no contexto atual. Útil para bibliotecas reutilizáveis. |

CARREGAR lib/utils.axis

CARREGAR lib/io.axis

## **2.8 Sistema e debug**

| **Comando**   | **Argumentos** | **Descrição**                                                                             |
| ------------- | -------------- | ----------------------------------------------------------------------------------------- |
| **SAIR**      | -              | Encerra o interpretador imediatamente com código de saída 0.                              |
| **DEBUG ★**   | -              | Ativa o modo verbose: cada comando imprime \[DBG\] CMD \| PTR \| VALOR antes de executar. |
| **NODEBUG ★** | -              | Desativa o modo verbose ativado por DEBUG.                                                |

O modo verbose também pode ser ativado pela linha de comando:

./axis programa.axis -v

Saída do modo debug:

\[DBG\] CMD: MAIS | PTR=0003 | VALOR=064

# **3\. Exemplos Completos**

## **3.1 Olá Mundo**

    FALAR "Olá, Mundo!"

    SAIR

## **3.2 Contador de 1 a 10**

    NOMEAR 0 contador

    ROTULO loop

    SE_IGUAL 11 fim

    VALOR

    MAIS

    PULAR loop

    ROTULO fim

    SAIR

## **3.3 Lê número e classifica**

    ENTRADA

    SE_MENOR 50 pequeno

    SE_MAIOR 100 grande

    FALAR "Entre 50 e 100"

    PULAR fim

    ROTULO pequeno

    FALAR "Menor que 50"

    PULAR fim

    ROTULO grande

    FALAR "Maior que 100"

    ROTULO fim

    SAIR

## **3.4 Imprimir alfabeto (A-Z)**

    NOMEAR 0 letra

    REPETE 65 \[MAIS\] \$ começa no 'A'

    ROTULO loop

    SE_MAIOR 90 fim \$ para depois do 'Z'

    FALAR

    MAIS

    PULAR loop

    ROTULO fim

    SAIR

## **3.5 Cópia de valor entre células**

    NOMEAR 0 origem

    NOMEAR 5 destino

    IR origem

    SOMA 99

    COPIAR 0 5 \$ destino = origem = 99

    IR destino

    VALOR \$ imprime 99

    SAIR

# **4\. Limites e Mensagens de Erro**

## **Limites do interpretador**

| **Comando**             | **Argumentos**           | **Descrição** |
| ----------------------- | ------------------------ | ------------- |
| **Tamanho da fita**     | 30.000 células (0-29999) |               |
| **Máximo de rótulos**   | 512                      |               |
| **Máximo de variáveis** | 512                      |               |
| **Tamanho de nome**     | 63 caracteres            |               |
| **Tamanho de linha**    | 511 caracteres           |               |

## **Mensagens comuns**

| **Comando**                                | **Argumentos** | **Descrição**                                |
| ------------------------------------------ | -------------- | -------------------------------------------- |
| **ERRO AXIS: Rotulo 'X' nao encontrado!**  |                | Rótulo usado sem declaração prévia.          |
| **ERRO AXIS: Posicao invalida em IR: N**   |                | IR apontou para fora do intervalo 0-29999.   |
| **ERRO AXIS: Limite de rotulos atingido!** |                | Mais de 512 rótulos no arquivo.              |
| **AVISO AXIS: Limite direito da fita!**    |                | DIREITA tentou ultrapassar a posição 29999.  |
| **AVISO AXIS: Comando desconhecido: 'X'**  |                | Token não reconhecido na tabela de comandos. |

# **5\. Guia Rápido (Cheat Sheet)**

| **Comando**            | **Argumentos** | **Descrição**                       |
| ---------------------- | -------------- | ----------------------------------- |
| **MAIS**               | -              | Célula ativa +1                     |
| **MENOS**              | -              | Célula ativa −1                     |
| **ZERAR**              | -              | Célula ativa = 0                    |
| **SOMA n**             | n: inteiro     | Célula ativa += n                   |
| **SUBTRAIR n**         | n: inteiro     | Célula ativa -= n                   |
| **DIREITA**            | -              | Ponteiro → direita                  |
| **ESQUERDA**           | -              | Ponteiro → esquerda                 |
| **IR pos\|nome**       | pos ou nome    | Move ponteiro para posição/variável |
| **COPIAR orig dest ★** | 0-29999        | fita\[dest\] = fita\[orig\]         |
| **NOMEAR pos nome**    | pos, nome      | Cria apelido para posição           |
| **ROTULO nome**        | nome           | Declara ponto de salto              |
| **PULAR nome**         | nome           | Salta para rótulo                   |
| **SE_IGUAL n rot**     | n, rótulo      | Salta se célula == n                |
| **SE_MENOR n rot ★**   | n, rótulo      | Salta se célula < n                 |
| **SE_MAIOR n rot ★**   | n, rótulo      | Salta se célula > n                 |
| **ENTRADA**            | -              | Lê inteiro do teclado               |
| **VALOR**              | -              | Imprime valor numérico              |
| **FALAR "txt"**        | string         | Imprime texto literal               |
| **FALAR**              | -              | Imprime char ASCII da célula        |
| **REPETE n \[cmds\]**  | n, comandos    | Executa bloco n vezes               |
| **CARREGAR arq**       | caminho        | Inclui arquivo externo              |
| **SAIR**               | -              | Encerra o programa                  |
| **DEBUG ★**            | -              | Ativa modo verbose                  |
| **NODEBUG ★**          | -              | Desativa modo verbose               |

★ = adição da versão 2.0

atualmente eu não sei o poder total dessa linguagem, se você conseguir fazer algo legal, ou tiver sugestões por favor me contate!!