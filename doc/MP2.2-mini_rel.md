# **Relatório Mini Projeto 2 - 1ª entrega**

## **Módulos principais**

### **Parse**

Implementa a função `parseCommand` que desempenha todo o processo de _parsing_ dos argumentos fornecidos através da linha de comandos, bem como a sua validação.

### **Log**

Contém a função `logEvent`, encarregue de escrever no terminal qualquer atualização que surja ao longo do programa, relativa à comunicação entre as _threads_ do cliente e o servidor. 

Além do evento em si - associado às _keywords_ `RECVD`, `TSKEX`, `TSKDN`, `2LATE` e `FAILD`, é também apresentada toda a informação relativa à _thread_ onde a ação se verificou.

### **Client**

Inclui todas as funções que abordam o fluxo principal do programa, _i.e._ a criação das _threads_ Produtoras e a escrita/leitura para _named pipes_ bem como do armazém.

Do conjunto de funções convém elencar: 

- `remainingTime`: retorna o número de segundos restantes de execução do programa, considerando o limite fornecido no terminal pelo utilizador.

- `fifoExists`: permite verificar a existência de um determinado ficheiro _FIFO_, para que comportamentos inesperados não ocorram ao tentar aceder ao mesmo.

- `writeToPrivateFifo`: caso seja possível, transmite a _struct Message_, gerada pelo cliente, através do _named pipe_ private fornecido, de modo a permitir a leitura por parte do cliente e o seu respectivo tratamento (retorna -1 em caso de erro).

    - Esta função dá uso a uma _struct pollfd_, que bloqueia a thread até que _named pipe_ reporte a sua disponibilidade para uma operação de _output_ (_flag POLLOUT_ em _events_, avaliando a resposta em _revents_), ou se dé o _timeout_ referido (neste caso o tempo restante do programa). Isto permite respeitar escrupulosamente os intervalos de tempo de forma bloqueante, evitando esperas ativas.

    - A escrita para o _named pipe_ é acompanhada de um _pthread_mutex_lock_, para que múltiplas escritas de diferentes _threads_ não origem qualquer situação de conflito,tendo em conta o elevado número de _threads_ suscetíveis de criação.

- `namePrivateFifo`: estão encarregues de nomear o _FIFO_ com o nome adequado (consoante a _thread_ em causa e localizado na pasta _/tmp_).

- `creatPublicFifo` e `removePublicFifo`: estão encarregues de criar com as permissões necessárias e de remover quando já não for útil, o public fifo com o nome passado como parametro da função.

- `readFromPublicFifo`: lê a informação enviada do servidor para o _named pipe_, colocando-a numa _struct Message_ referenciada por apontador. A _struct pollfd_ permite, desta vez, saber se existe alguma informação pronta a receber ou se um erro ocorreu entretanto (retornando -1). De qualquer forma, o _FIFO_ é encerrado e o espaço ocupado pela _struct Message_ é desalocado antes do término da função.

- `handleRequest`: esta função é chamada aquando da invocação de _pthread_create_, executando o seu papel realizando uma chamada á função fornecida na pasta lib, bem como a escrita do resultado numa queue. A função rege-se da seguinte forma: 

    1. Realiza a chamada á função task;

    2. Escreve o seu pedido no _named pipe_ público, de modo a ser atendido - `TSKEX`.

    3. Escreve a resposta obtida pela chamada anterior na _queue_, para uma futura leitura por parte da thread consumidora.

- `createStorage`: encarregue por criar a queue que irá ser usada como armazem.

- `writeMessageToStorage`: este função recebe um mensagem que deverá ser escrita no armazem. Para realizar esta operação fazemos uso de um _semaforo_, caso o espaço necessário disponivel escrita é realizada.

- `readMessageFromStorage`: este função tem o dever de ler, caso haja, informação no armazem. Sendo esta a operação contraria á leitura tambem ela contara com o auxilio de um semafora para realizar as suas operaçoes. Returnando no final a mensagem lida.

- `generateThreads`: lê sucessivamente a public fifo dentro do limite de tempo estabelecido, originando sucessivamente novas threads produtoras que iram realizar a tarefa. O _array_ de _pthread_t_ é vantajoso uma vez que permite a junção - _pthread_join_ - das _threads_ antes do retorno, para não deixar nenhuma órfã. O _loop_ atua enquanto existir um _named pipe_ o tempo não se esgutar, sendo atribuída uma _struct Message_ nova a cada _thread_ antes da sua criação. Por fim o seu _tid_ é armazenado no array. Apos estes passos a thread consumidora irá verificar se o armazem tem algo que possa ser lido, quando esta verificação terminar o loop repete-se. Por fim, o espaço alocado ao _array_ é libertado.

- `main`: responsável por ler e validar o comando proveniente do terminal, de inicializar e terminar o _mutex_ utilizado, abrir e fechar o _named pipe_ público e invocar a função `generateThreads`;
