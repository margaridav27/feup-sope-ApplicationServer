# **Relatório Mini Projeto 2 - 1ª entrega**

## **Módulos principais**

### **Parse**

Implementa a função `parseCommand` que desempenha todo o processo de _parsing_ dos argumentos fornecidos através da linha de comandos, bem como a sua validação.

### **Log**

Contém a função `logEvent`, encarregue de escrever no terminal qualquer atualização que surja ao longo do programa, relativa à comunicação entre as _threads_ do cliente e o servidor. 

Além do evento em si - associado às _keywords_ `IWANT`, `GOTRS`, `CLOSD` e `GAVUP`, é também apresentada toda a informação relativa à _thread_ onde a ação se verificou.

### **Client**

Inclui todas as funções que abordam o fluxo principal do programa, _i.e._ a criação das _threads_ responsáveis por cada pedido e a escrita/leitura para _named pipes_.

Do conjunto de funções convém elencar: 

- `remainingTime`: retorna o número de segundos restantes de execução do programa, considerando o limite fornecido no terminal pelo utilizador.

- `generateNumber`: gera um número aleatório entre um mínimo e máximo fornecidos.

- `fifoExists`: permite verificar a existência de um determinado ficheiro _FIFO_, para que comportamentos inesperados não ocorram ao tentar aceder ao mesmo.

- `writeToPublicFifo`: caso seja possível, transmite a _struct Message_, gerada pelo cliente, através do _named pipe_ público fornecido, de modo a permitir a leitura por parte do servidor e o seu respectivo tratamento (retorna -1 em caso de erro).

    - Esta função dá uso a uma _struct pollfd_, que bloqueia a thread até que _named pipe_ reporte a sua disponibilidade para uma operação de _output_ (_flag POLLOUT_ em _events_, avaliando a resposta em _revents_), ou se dé o _timeout_ referido (neste caso o tempo restante do programa). Isto permite respeitar escrupulosamente os intervalos de tempo de forma bloqueante, evitando esperas ativas.

    - A escrita para o _named pipe_ é acompanhada de um _pthread_mutex_lock_, para que múltiplas escritas de diferentes _threads_ não origem qualquer situação de conflito,tendo em conta o elevado número de _threads_ suscetíveis de criação.

- `namePrivateFifo`, `creatPrivateFifo` e `removePrivateFifo`: estão encarregues de nomear o _FIFO_ com o nome adequado (consoante a _thread_ em causa e localizado na pasta _/tmp_), de criá-lo com as permissões necessárias e de removê-lo quando já não for útil.

- `readFromPrivateFifo`: lê a informação enviada do servidor para o _named pipe_, colocando-a numa _struct Message_ referenciada por apontador. A _struct pollfd_ permite, desta vez, saber se existe alguma informação pronta a receber ou se um erro ocorreu entretanto (retornando -1). De qualquer forma, o _FIFO_ é encerrado e o espaço ocupado pela _struct Message_ é desalocado antes do término da função.

- `generateRequest`: esta função é chamada aquando da invocação de _pthread_create_, executando o seu papel principal recorrendo às funções mencionadas para concretizar os processos de escrita e leitura nas _FIFOs_ e o registo pretendido. A função rege-se da seguinte forma: 

    1. Cria o _named pipe_ privado, para que o servidor possa responder;
    2. Escreve o seu pedido no _named pipe_ público, de modo a ser atendido - `IWANT`.

    3. Lê a possível resposta do servidor no _named pipe_ privado. Se, por algum motivo, o cliente já não puder esperar pela resposta do servidor, desiste - `GAVUP`. Por outro lado, se esta operação for bem sucedida mas a resposta do servidor corresponder a -1, significa que este encerrou antes de poder completar o pedido - `CLOSD`.

    4. Se nenhuma das situações anteriores se verificar, o servidor realizou a tarefa pretendida e respondeu de forma válida - `GOTRS`.

    5. Independentemente da ocorrência de erros, a função garante que, tanto o _FIFO_, como a _struct Message_, são devidamente tratados, evitando _leaks_.

- `assembleMessage`: preenche os campos de uma dada _struct Message_, com a informação relativa à _thread atual_ e o seu respectivo número de identificação, passado como argumento;

- `generateThreads`: gera _threads_ em intervalos aleatórios dentro do limite de tempo estabelecido, originando sucessivamente novos pedidos a ser enviados ao servidor. O _array_ de _pthread_t_ é vantajoso uma vez que permite a junção - _pthread_join_ - das _threads_ antes do retorno, para não deixar nenhuma órfã. O _loop_ atua enquanto existir um _named pipe_ público ativo, _i.e._ o servidor ainda está disponível para receber novas tarefas, sendo atribuída uma _struct Message_ nova a cada _thread_ antes da sua criação. Por fim o seu _tid_ é armazenado no array e uma pequena espera é feita, antes de repetir os passos de novo. Por fim, o espaço alocado ao _array_ é libertado.

- `main`: responsável por inicializar a _seed_ que gera qualquer número aleatório ao longo do programa, de ler e validar o comando proveniente do terminal, de inicializar e terminar o _mutex_ utilizado, abrir e fechar o _named pipe_ público e invocar a função `generateThreads`;
