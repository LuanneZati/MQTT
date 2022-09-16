# MQTT

Desenvolvimento de projeto IoT
Nesse projeto, ainda em fase de testes, o objetivo é desenvolver um simples dispositivo IoT em cloud.
Está sendo usado: ESP32 - AWS - MQTT Broker - python - Banco de dados RDS (MariaDB) - Grafana.

Basicamente, foi desenvolvido um código na Arduino IDE, gravado no ESP, que pega as informações do sensor de temperatura da placa e envia para um tópico no MQTT Broker(cloud). O código em python se inscreve no tópico que esta sendo enviado a mensagem de temperatura, capta esse payload e o armazena no banco de dados(cloud).
