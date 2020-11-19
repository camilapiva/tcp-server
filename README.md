
Socket TCP Server
=============
Este projeto é uma aplicação desenvolvida de que disponibiliza as informações de temperatura, umidade e distância através de uma conexão socket TCP fornecida pelo ESP32. Sendo assim, se faz compreender o exemplo de socket TCP server fornecido pelo esp-idf utilizando este recurso.

Requisitos 

Módulo WiFi ESP32 com Display Oled 

ESP-IDF (Espressif IoT Development Framework)

RealTerm: Serial/TCP Terminal



Procedimentos no ESP-IDF (Espressif IoT Development Framework)

1º Passo: inserir esse projeto dentro do diretório do esp-idf.

2º Passo: Abrir o software ESP-IDF e entrar dentro do diretório onde se localiza o projeto, utilizando o seguinte comando: cd <diretório do esp-idf>\esp-idf\examples\protocols\sockets\tcp_server

3º Passo: Executar o comando: idf.py menuconfig

4º Passo: Ao abrir a janela de configuração, navegue até o item Serial flasher config, depois acesse o item Flash size dê um Enter no teclado e escolha a opção 4 MB. Depois clique duas vezes em Esc para voltar ao menu principal. 

4º Passo: navegue até o item Example configuration. Nesse item será configurado a porta de conexão do servidor. 

5º Passo: acesse o item da configuração Example connection configuration, esse item será configurado para a conexão de WiFi. 

6º Passo: Clique no botão S do teclado para salvar as configurações e depois em Esc para sair da janela de configurações.

7º Passo: Para fazer a build do projeto, execute o comando: idf.py flash monitor


Procedimentos no RealTerm: Serial/TCP Terminal

1º Passo: Abra o RealTerm.

2º Passo: acesse a aba Port.

3º Passo: no item port insira o endereço de ip e a porta que é mostrada no terminal esp-idf ao conectar com a rede wifi. Exemplo: 192.168.0.1:3333

4º Passo: Clique em open.

5º Passo: Acesse a aba Send. 

6º Passo: No primeiro Dropdown, insira uma das letras (t -> obter o valor da temperatura; u -> obter o valor da umidade; d -> para obter o valor da distância), depois clique em Send ASCII ao lado do dropdown.

