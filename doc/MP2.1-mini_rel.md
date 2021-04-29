# Relatório Mini Projeto 2 - 1ª entrega

## Módulos principais

### Parse

Implementa a função **_parseCommand_** que desempenha todo o processo de _parsing_ dos argumentos fornecidos através da linha de comandos, bem como a sua validação.

### Log

Contém a função **_logEvent_**, encarregue de escrever no terminal qualquer atualização que surja ao longo do programa, relativa à comunicação entre as _threads_ do cliente e o servidor. 

Além do evento em si - associado às _keywords_ **_IWANT_**, **_GOTRS_**, **_CLOSD_** e **_GAVUP_** -, é também apresentada toda a informação relativa à _thread_ onde a ação se verificou.

### Client

Inclui todas as funções que abordam o fluxo principal do programa, _i.e._ a criação das _threads_ responsáveis por cada pedido e a escrita/leitura para _named pipes_.

Do conjunto de funções convém elencar: 

- **_remainingTime_**: retorna o número de segundos restantes de execução do programa, considerando o limite fornecido no terminal pelo utilizador.

- **_generateNumber_**: gera um número aleatório entre um mínimo e máximo fornecidos.

- **_fifoExists_**: permite verificar a existência de um determinado ficheiro _FIFO_, para que comportamentos inesperados não ocorram ao tentar aceder ao mesmo.

- **_writeToPublicFifo_**: caso seja possível, transmite a _struct Message_, gerada pelo cliente, através do _named pipe_ público fornecido, de modo a permitir a leitura por parte do servidor e o seu respectivo tratamento (retorna -1 em caso de erro).

    - Esta função dá uso a uma _struct pollfd_, que interpela o _named pipe_ para averiguar da sua disponibilidade para uma operação de _output_, neste caso, escrita de forma não bloqueante (_flag POLLOUT_ em _events_), e indica se tal operação foi bem sucedida, avaliando a resposta em _revents_. Permite, ainda, manter a comunicação aberta consoante um _timeout_ referido, neste caso o tempo restante do programa.

    - A escrita para o _named pipe_ é acompanhada de um _pthread_mutex_lock_, para que múltiplas escritas de diferentes _threads_ não origem qualquer situação de conflito,tendo em conta o elevado número de _threads_ suscetíveis de criação.

- **_namePrivateFifo_**, **_creatPrivateFifo_** e **_removePrivateFifo_**: estão encarregues de nomear o _FIFO_ com o nome adequado (consoante a _thread_ em causa e localizado na pasta _/tmp_), de criá-lo com as permissões necessárias e de removê-lo quando já não for útil.

- **_readFromPrivateFifo_**: lê a informação enviada do servidor para o _named pipe_, colocando-a numa _struct Message_ referenciada por apontador. A _struct pollfd_ permite, desta vez, saber se existe alguma informação pronta a receber ou se um erro ocorreu entretanto (retornando -1). De qualquer forma, o _FIFO_ é encerrado e o espaço ocupado pela _struct Message_ é desalocado antes do término da função.

- **_generateRequest_**: esta função é chamada aquando da invocação de _pthread_create_, executando o seu papel principal recorrendo às funções mencionadas para concretizar os processos de escrita e leitura nas _FIFOs_ e o registo pretendido. A função rege-se da seguinte forma: 

    1. Cria o _named pipe_ privado, para que o servidor possa responder;
    2. Escreve o seu pedido no _named pipe_ público, de modo a ser atendido - **_IWANT_**.

    3. Lê a possível resposta do servidor no _named pipe_ privado. Se, por algum motivo, o cliente já não puder esperar pela resposta do servidor, desiste - **_GAVUP_**. Por outro lado, se esta operação for bem sucedida mas a resposta do servidor corresponder a -1, significa que este encerrou antes de poder completar o pedido - **_CLOSD_**.

    4. Se nenhuma das situações anteriores se verificar, o servidor realizou a tarefa pretendida e respondeu de forma válida - **_GOTRS_**.

    5. Independentemente da ocorrência de erros, a função garante que, tanto o _FIFO_, como a _struct Message_, são devidamente tratados, evitando _leaks_.

- **_assembleMessage_**: preenche os campos de uma dada _struct Message_, com a informação relativa à _thread atual_ e o seu respectivo número de identificação, passado como argumento;

- **_generateThreads_**: gera _threads_ em intervalos aleatórios dentro do limite de tempo estabelecido, originando sucessivamente novos pedidos a ser enviados ao servidor. O _array_ de _pthread_t_ é vantajoso uma vez que permite a junção - _pthread_join_ - das _threads_ antes do retorno, para não deixar nenhuma órfã. O _loop_ atua enquanto existir um _named pipe_ público ativo, _i.e._ o servidor ainda está disponível para receber novas tarefas, sendo atribuída uma _struct Message_ nova a cada _thread_ antes da sua criação. Por fim o seu _tid_ é armazenado no array e uma pequena espera é feita, antes de repetir os passos de novo. Por fim, o espaço alocado ao _array_ é libertado.

- **_main_**: responsável por inicializar a _seed_ que gera qualquer número aleatório ao longo do programa, de ler e validar o comando proveniente do terminal, de inicializar e terminar o _mutex_ utilizado, abrir e fechar o _named pipe_ público e invocar a função **_generateThreads_**;