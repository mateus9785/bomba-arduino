# bomba-arduino

Simulador de bomba para escape room: um timer regressivo de 4 dígitos combinado com um jogo de "corte o fio certo", rodando em Arduino.

## Funcionalidades

- Timer regressivo de 4 dígitos (MM:SS) em display de 7 segmentos, controlado por dois registradores de deslocamento 74HC595.
- Leitura de 8 fios via registrador de deslocamento de entrada 74HC165, detectando o corte de cada fio individualmente.
- Lógica que calcula qual é o próximo fio correto a cortar a cada rodada, considerando cor, espessura (fino/grosso) e qual foi o último fio cortado.
- Cortar o fio errado reduz o timer em 5 minutos, ativa os LEDs de erro e toca um som de erro.
- Cortar todos os fios corretos antes do tempo acabar acende o LED verde de sucesso e para o timer.
- Se o tempo chega a zero, dispara um alarme sonoro contínuo e aciona o pino do "detonador" uma única vez.
- Log detalhado via serial: estado de cada fio, leitura bruta dos pinos do 74HC165 e qual fio deveria ser cortado a seguir.

## Hardware

- 2x 74HC595 (registradores de deslocamento de saída): um para os segmentos do display, outro para selecionar o dígito ativo.
- 1x 74HC165 (registrador de deslocamento de entrada): leitura dos 8 fios.
- Buzzer no pino 10.
- Pino do "detonador" no pino 9.
- 1 LED verde de sucesso e 3 LEDs de erro.
- 8 fios de cores e espessuras diferentes (preto, roxo, amarelo, laranja, vermelho, cada um fino ou grosso) ligados às entradas do 74HC165.

O mapeamento completo de pinos está comentado no início de `nova_bomba_v7.ino`.

## Arquivo de teste

`test_getCorrectWireIndex.ino` é um sketch separado que testa só a lógica de decisão de qual fio cortar (`getCorrectWireIndex()`), sem precisar do circuito montado. Ele roda via monitor serial e aceita comandos:

- `c0` a `c7`: corta o fio pelo índice.
- `r0` a `r7`: reconecta o fio pelo índice.
- `last0` a `last7`: define qual foi o último fio cortado.
- `reset`: reconecta todos os fios.
- `state`: imprime o estado atual de todos os fios e qual seria o próximo fio correto.

## Como usar

1. Abra `nova_bomba_v7.ino` na Arduino IDE.
2. Monte o circuito seguindo os pinos definidos no início do arquivo (display via 74HC595, fios via 74HC165, buzzer, LEDs e detonador).
3. Faça upload do sketch para a placa.
4. Abra o monitor serial em 9600 baud para acompanhar o log de eventos (fio cortado, acerto/erro, ativação do alarme).

## Limitações conhecidas

- Toda a lógica de `getCorrectWireIndex()` está concentrada em uma função grande, com um bloco de regras condicionais separado por quantidade de fios restantes (8 a 1). Não há testes automatizados; a validação é feita manualmente pelo sketch de teste via comandos seriais.
- Não há persistência de estado: reiniciar a placa reinicia o timer e reconecta todos os fios.
- Projeto contido em dois arquivos `.ino`, sem separação em bibliotecas ou módulos.
