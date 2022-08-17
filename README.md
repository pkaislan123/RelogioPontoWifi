# Relogio de Ponto Wifi


O Código a seguir é usado num NodeMcu, que fica responsavel pelo registro de ponto dos relogios de pontos fabricados pela TItaniwm.

O sistema eletrônico desses relógios consiste em:

-> Um display 20x4 i2c para Interface IHM

-> Um teclado matricial 4x4 para Interface IHM, nele é possível efetuar operações como registro de ponto por CPF, reinicio do sistema, reinicio da alimentação e reinicio do wifi.

-> Um sensor RFID para leitura da tag ou cartão de ponto

-> Um buzzer para respostas usando efeito sonoro

-> Um módulo NodeMcu Esp8266 atuando como placa mãe

<p align="center">
  <img src=https://raw.githubusercontent.com/pkaislan123/ControleEnvazamentoComArduino/main/1.png title="hover text">
</p>
