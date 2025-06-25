# Trabalho Prático IoT - Detecção de movimentação suspeita

![Static Badge](https://img.shields.io/badge/GCC271-UFLA-green)
![Static Badge](https://img.shields.io/badge/2025%2F1-gray)

Universidade Federal de Lavras

Prof. Hermes Pimenta

Prof. João Giacomin

## Descrição

Este trabalho tem por objetivo desenvolver uma aplicação IoT completa. Como tema geral para o trabalho, é proposto **Campus Inteligente**. Dentro desse contexto, diversos temas menores foram propostos. Como tema específico para este trabalho, é proposto **detecção de movimentação suspeita**.

Requisitos:

🔹 Utilizar o microcontrolador ESP32;

🔹 Desenvolver uma aplicação acessível da internet;

🔹 Os dados gerados pelos ESP também devem estar disponíveis na internet;

🔹 Envio de dados pode ocorrer por meio de um protocolo de comunicação (e.g. MQTT), diretamente a um banco de dados/aplicação;

🔹 A aplicação deve ser capaz de ler e exibir os dados recebidos do ESP e enviar comandos a esse microcontrolador;

🔹 A aplicação pode ser desenvolvida como uma aplicação WEB, do zero, ou utilizar ferramentas de Dashboard.

## Diretórios

📁 A pasta `projeto` contém a implementação do servidor Flask, o banco de dados SQLite e as imagens salvas tiradas pelas câmeras.

📁 A pasta `codigos` contém a implementação dos códigos utilizados nos microcontroladores ESP32.

## Como executar

### **Pré-requisitos**

É preciso ter as seguintes ferramentas instaladas.

- [Python](https://www.python.org/downloads/)
- [Git](https://git-scm.com/)
- [Arduino IDE](https://www.arduino.cc/en/software/)

Os microcontroladores utilizados para este trabalho foram:

- Um Heltec LoRa 32 (V2)
- Dois ESP32-CAM com suporte ES32-CAM-MB

### **Passo 1: Clonar o Repositório**

Clone este repositório para o seu ambiente local usando o Git.

```bash
git clone https://github.com/brunof5/GCC271-Trab_IoT.git
```

### **Passo 2: Criar o Ambiente Virtual**

Preferencialmente, dentro do diretório `projeto`, crie um ambiente virtual.

- **Windows**:
  
  ```bash
  python -m venv .venv
  ```
  
- **Linux/MacOS**:
  
  ```bash
  python3 -m venv .venv
  ```

### **Passo 3: Ativar o Ambiente Virtual**

- **Windows**:
  
  ```bash
  .venv\Scripts\activate
  ```

- **Linux/MacOS**:

  ```bash
  source .venv/bin/activate
  ```

### **Passo 4: Instalar as Dependências**

Instale as dependências necessárias usando o `pip`.

```bash
pip install Flask
```

### **Passo 5: Executar o Flask**

**Atenção**: ao executar o Flask dessa maneira, o servidor será exposto para os dispositivos conectados na mesma rede local, utilize somente se você confia nos outros usuário da sua rede.

```bash
flask run --host=0.0.0.0
```
