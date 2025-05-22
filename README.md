# 💧 Monitor de Qualidade da Água - *Water Quality Monitor*

Este projeto realiza a **análise em tempo real da qualidade da água**, verificando diversos parâmetros físicos e químicos. Os dados são coletados por sensores conectados a um **Arduino Mega**, enviados via **Wi-Fi com ESP-01 (ESP8266)** para uma **API .NET**, e armazenados em um **banco de dados MongoDB**.

---

## 📌 Visão Geral

O sistema tem como objetivo **monitorar a qualidade da água** para diferentes aplicações, como:

- Consumo humano
- Ambientes controlados (ex: tanques de peixes)
- Monitoramento ambiental

---

## 🔧 Componentes Principais

### 🧠 Microcontrolador
- **Arduino Mega**  
  Responsável pela leitura dos sensores e comunicação serial com o módulo Wi-Fi.

### 📶 Comunicação
- **ESP-01 (ESP8266)**  
  Módulo Wi-Fi utilizado para comunicação TCP com a API REST.

### 📟 Interface
- **Display LCD I2C**  
  Exibe mensagens de status e erros diretamente no sistema.

### 🌡️ Sensores
- **Sensor de Temperatura**
- **Sensor DHT11** – Umidade e temperatura do ar
- **Sensor de pH** – Potencial hidrogeniônico da água
- **Sensor TDS** – Sólidos dissolvidos totais

---

## 🖥️ Backend

- **API REST** desenvolvida em **.NET**
- Armazena os dados enviados pelos sensores
- Realiza análises, disponibiliza relatórios e gráficos (em expansão)

---

## 💾 Banco de Dados

- **MongoDB**  
  Utilizado para armazenar os dados dos sensores com flexibilidade e performance.

---

## 📈 Arquitetura do Sistema

```mermaid
graph TD
    SensorTemp[Sensor Temperatura] --> Arduino
    SensorDHT11[DHT11] --> Arduino
    SensorpH[pH] --> Arduino
    SensorTDS[TDS] --> Arduino
    Arduino -->|Serial1| ESP01
    ESP01 -->|TCP/IP| API[API .NET]
    API --> MongoDB[(MongoDB)]
    Arduino --> LCD[LCD I2C]
