#ifndef __DHT_H__
#define __DHT_H__

#include <stm32f1xx.h>
#include <time.h>

#define DHT_DATA 5
enum DHT_type {
    DHT11 = 0,
    DHT12,
    DHT21,
    DHT22
};

struct DHT_t {
  uint8_t  data[DHT_DATA];  // принимаемые пакеты
  GPIO_TypeDef*  port;      // порт выхода
  uint8_t  pin;             // номер выхода
  //uint8_t  pin_mode;        // маска mode выхода
  //uint8_t  pin_cnf;         // маска cnf выхода
  enum DHT_type  type;      // тип DHT
  uint32_t last_read_time;  // время последнего считывания (период в 2000 мс)
  uint32_t max_cycles;      // максимальная длина сигнала с DHT
  int8_t   result;          // корректность последнего считывания
  uint8_t  pull_time;       // время подготовки данных на DHT мс
};

struct DHT_t DHT_init(GPIO_TypeDef*  port, uint8_t pin, enum DHT_type type);

int8_t DHT_begin(struct DHT_t *dht); // (0 - нет ошибки)

int8_t DHT_check_sensor(struct DHT_t *dht); // команда считывания данных, возврат - корректность (0 - нет ошибки)
int8_t DHT_check_result(struct DHT_t *dht); // возврат - корректность (0 - нет ошибки)

float DHT_get_temperature(struct DHT_t *dht); // C
float DHT_get_humidity(struct DHT_t *dht);    // %

#endif