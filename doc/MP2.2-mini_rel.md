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

- `readFromPublicFifo`: lê a informação enviada por uma thread do cliente para o _named pipe_ público, colocando-a
  numa _struct Message_ referenciada por apontador. A _struct pollfd_ permite, desta vez, saber se existe alguma
  informação pronta a receber ou se um erro ocorreu entretanto (retornando -1).

    - O _mutex_ de leitura garante que a leitura do _fifo_ público seja de forma ordenada e intercalada entre threads
      para não ocorrem conflitos de acesso, originando informação corrompida. Em caso de retorno por erro, é desativado
      de forma a não perturbar o seu futuro funcionamento.

- `handleRequest`: esta função é chamada aquando da invocação de _pthread_create_, executando o seu papel realizando uma chamada á função fornecida na pasta lib, bem como a escrita do resultado numa queue. A função rege-se da seguinte forma: 

    1. Realiza a chamada á função task;

    2. Escreve o seu pedido no _named pipe_ público, de modo a ser atendido - `TSKEX`.

    3. Escreve a resposta obtida pela chamada anterior na _queue_, para uma futura leitura por parte da thread consumidora.

- O armazenamento de Mensagens já executadas pelos servidor é realizado através de uma Singly Linked Tail Queue (
incluída em sys/queue.h). Recorrendo a várias funções que a estrutura fornece, como STAILQ_INSERT_TAIL, STAILQ_FIRST e
STAILQ_REMOVE_HEAD, é possível inserir novos nós (que incluem a Mensagem e a lista de entrada para o nó seguinte) no
fim da fila e retirar o nó inserido há mais tempo do início da fila, garantindo o comportamento _FIFO_.

- De forma a garantir uma coordenação entre a escrita e a leitura no _buffer_ e para que não seja ultrapassado o limite
de mensagens fornecido no argumento da execução, dois semáforos são iniciados empty com o tamanho do buffer e
representado o número de slots vazias que podem ser ocupadas por mensagens e full iniciado a 0, que representa as
slots já ocupadas da fila.

- `createStorage`: encarregue por criar a queue que irá ser usada como armazem.

- `writeMessageToStorage`: chamada numa thread produtora, cria um novo nó com a mensagem fornecida como argumento e
insere-o no final da queue de armazenamento. Verifica a condição do semáforo empty, garantindo que existe pelo menos
uma posição livre na fila que pode ser ocupada com o novo nó, decrementando-o o semáforo. Antes de retornar aumenta o
semáforo full, visto que uma mensagem preencheu a última posição da fila.

- `readMessageFromStorage`: chamada numa thread consumidora, tem como tarefa ler a mensagem mais antiga da fila de
armazenamento para que possa ser posteriormente escrita na FIFO previda correspondente. Se não existir qualquer
mensagem para leitura, retorna 1. Verifica a condição do semáforo full, garantindo que existe algo para ser lido e
decrementando-o. Antes de retornar aumenta o semáforo empty, visto que uma mensagem foi retirada da fila.

- `consumeTask`: invocada na criação da thread consumidora. Tem como única responsabilidade ler regularmente a fila de
  armazenamento de mensagens, retirando uma mensagem a cada iteração do seu _loop_ principal e escrevendo-a para o _
  FIFO_ de forma a ser recebida do lado do cliente.

- `generateThreads`: Esta função tem o papel de gerar os dois tipos de threads necessárias à execução do programa. Em
  primeiro lugar, gera a thread consumidora (única para todo o programa) que interliga o _buffer_ de armazenamento e a
  envio de um dado pedido para o lado do cliente. Por outro lado, no caso de se verificar que existe algo para ser lido
  na estrutura _fifo_ públic, gera uma thread produtora que irá processar o _request_ e escrever a Mensagem já com a
  resposta na fila, registando a sua receção com _RECVD_. O _array_ de _pthread_t_ é vantajoso uma vez que permite a
  junção - _pthread_join_ - das _threads_ antes do retorno, para não deixar nenhuma órfã. Para tal a cada geração de uma
  thread produtora, o seu _tid_ é registado. A _thread_ consumidora é também reunida com  _pthread_join_. Antes de
  retornar, esta função liberta todo o espaço em memória necessário à sua execução.


- `main`: responsável por ler e validar o comando proveniente do terminal, de inicializar e terminar o _mutex_ utilizado, abrir e fechar o _named pipe_ público e invocar a função `generateThreads`;
