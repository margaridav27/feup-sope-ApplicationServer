## **Módulos principais**

### **Parse**

Implementa a função `parseCommand` que desempenha todo o processo de _parsing_ dos argumentos fornecidos através da
linha de comandos, bem como a sua validação.

### **Log**

Contém a função `logEvent`, encarregue de escrever no terminal qualquer atualização que surja ao longo do programa,
relativa à comunicação entre as _threads_ do cliente e o servidor.

Além do evento em si - associado às _keywords_ `RECVD`, `TSKEX`, `TSKDN`, `FAILD` e `_2LATE`, é também apresentada toda
a informação relativa à _thread_ onde a ação se verificou.

### **Server**

Inclui todas as funções que abordam o fluxo principal do programa, _i.e._ a criação das _threads_ responsáveis por cada
pedido e a escrita/leitura para _named pipes_.

Do conjunto de funções convém elencar:

- `remainingTime`: retorna o número de segundos restantes de execução do programa, considerando o limite fornecido no
  terminal pelo utilizador.

- `fifoExists`: permite verificar a existência de um determinado ficheiro _FIFO_, para que comportamentos inesperados
  não ocorram ao tentar aceder ao mesmo.

- `namePrivateFifo`: através da informação de cada mensagem, o servidor conseguirá saber qual é o _fifo_ ao qual deve
  enviar a resposta a um dado pedido, sendo necessário definir o seu nome.


- `writeToPrivateFifo`: caso seja possível, devolve a _struct Message_ com a resposta do servidor, através do _named
  pipe_ privado fornecido, para que este analise o resultado fornecido pela execução do pedido ou então de qualquer
  outra ocorrência caso o valor desse atributo seja -1. Antes de retornar, regista o evento como _TSKDN_. A flag
  POLL_HUP, permite saber se existiu uma leitura válida (não interrompida) de uma Mensagem, de forma a não ser escrita
  uma mensagem sem conteúdo. No caso de não ser possível realizar uma escrita regista o evento como _FAILD_ e se o
  servidor não teve oportunidade de responder (resposta a -1) _2LATE_.

    - Esta função dá uso a uma _struct pollfd_, que bloqueia a thread até que _named pipe_ reporte a sua disponibilidade
      para uma operação de _output_ (_flag POLLOUT_ em _events_, avaliando a resposta em _revents_), ou se dé o _
      timeout_ referido (neste caso o tempo restante do programa). Isto permite respeitar escrupulosamente os intervalos
      de tempo de forma bloqueante, evitando esperas ativas.


- `readFromPublicFifo`: após o servidor receber uma mensagem de uma _thread_ específica do cliente no _named pipe_
  público, esta é lida e inserida numa _struct Message_ referenciada por apontador. A _struct pollfd_ permite, desta
  vez, saber se existe alguma informação pronta a receber ou se um erro ocorreu entretanto (retornando -1).

    - O _mutex_ de leitura garante que a leitura do _fifo_ público é realizada de forma ordenada e intercalada entre
      _threads_. Recorrendo a este método, não ocorrem conflitos de acesso, que poderiam originar informação corrompida.
      Em caso de retorno por erro, é desativado de forma a não prejudicar uma utilização futura.

- `Armazenamento`: o armazenamento de Mensagens já executadas pelo servidor é realizado através de uma Singly Linked
  Tail Queue (incluída em sys/queue.h). Recorrendo a várias funções que a estrutura fornece, como _STAILQ_INSERT_TAIL_,
  _STAILQ_FIRST_ e _STAILQ_REMOVE_HEAD_, é possível inserir novos nós (que incluem a Mensagem e a lista de entrada para
  o nó seguinte) no fim da fila e retirar o nó inserido há mais tempo do início da fila, recriando o comportamento
  habitual de uma estrutura _FIFO_ para organizar as mensagens.

- `Semaphores`: para coordenar a escrita e a leitura no _buffer_ e para que não seja ultrapassado o limite de mensagens
  fornecido no argumento da consola, dois semáforos são iniciados: **empty** com o tamanho do buffer, representa o
  número de espaços vazios que podem ser ocupadas por mensagens e **full** iniciado a 0, indicando as posições já
  ocupadas da fila.

- `writeMessageToStorage`: chamada numa thread produtora, cria um novo nó com a mensagem fornecida como atributo e
  insere-o no final da queue de armazenamento. Antes de efetuar qualquer inserção, verifica a condição do semáforo
  _empty_, garantindo que existe pelo menos uma posição livre na fila que pode ser ocupada com o novo nó,
  decrementando-o o semáforo. Antes de retornar aumenta o semáforo _full_, visto que uma mensagem preencheu a uma das
  posições da fila.

  -`readMessageFromStorage`: chamada numa thread consumidora, lê a mensagem inserida há mais tempo na fila de
  armazenamento copiando-a para uma _Message_ passada como argumento, para que possa ser posteriormente escrita na _
  FIFO_ correspondente. Se não existir qualquer mensagem para leitura, retorna 1. Verifica a condição do semáforo _full_
  , garantindo que existe algo para ser lido e decrementa-o. Antes de retornar aumenta o semáforo _empty_, notificando o
  programa de que uma mensagem foi retirada da fila.

- `consumeTask`: invocada na criação da thread consumidora. Tem como única responsabilidade ler regularmente a fila de
  armazenamento de mensagens (até ao tempo indicado), retirando uma mensagem a cada iteração do seu _loop_ principal e
  escrevendo-a na _FIFO_ privada de forma a ser recebida do lado do cliente. Antes de terminar, liberta a memória
  associada à mensagem.

- `handleRequest`: função invocada durante a criação de uma nova thread produtora. Quando surge uma nova mensagem na
  _FIFO_ pública cabe a esta função proceder à sua execução (com a função **task** já fornecida), o registo da sua
  execução com _TSKEX_ e escrevendo a mensagem, já atualizada com a respetiva resposta para o buffer/storage.


- `generateThreads`: Esta função tem o papel de gerar os dois tipos de threads necessárias à execução do programa. Em
  primeiro lugar, gera a thread consumidora (única para todo o programa) que interliga o _buffer_ de armazenamento e a
  envio de um dado pedido para o lado do cliente. Por outro lado, no caso de se verificar que existe algo para ser lido
  na estrutura _fifo_ públic, gera uma thread produtora que irá processar o _request_ e escrever a Mensagem já com a
  resposta na fila, registando a sua receção com _RECVD_. O _array_ de _pthread_t_ é vantajoso uma vez que permite a
  junção - _pthread_join_ - das _threads_ antes do retorno, para não deixar nenhuma órfã. Para tal a cada geração de uma
  thread produtora, o seu _tid_ é registado. A _thread_ consumidora é também reunida com  _pthread_join_. Antes de
  retornar, esta função liberta todo o espaço em memória necessário à sua execução.


- `main`: inicia a _queue_ de armazenamento com um tamanho _default_ ou o que foi inserido na consola, lê e valida o
  comando proveniente do terminal, inicia e termina o
  _mutex_ e os _semaphores_ utilizados, abre, fecha e remove o _named pipe_ público e invocar a função `generateThreads`
  ;
