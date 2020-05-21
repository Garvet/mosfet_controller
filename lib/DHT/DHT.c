#include "DHT.h"

// - pin -
// -----------------------------------------------------------------
// | name                     | CNF1 | CNF0 | MODE1 | MODE0 | ODR  |
// |-------------|------------|------|------|---------------|------|
// | output prog | Push-pull  |   0  |   0  |               | 0or1 |
// |             | Open-drain |   0  |   1  |      0 1      | 0or1 |
// |-------------|------------|------|------|      1 0      |------|
// | output auto | Push-pull  |   1  |   0  |      1 1      |  -   |
// |             | Open-drain |   1  |   1  |               |  -   |
// |-------------|------------|------|------|---------------|------|
// | input       | Analog     |   0  |   0  |               |  -   |
// |             | Floating   |   0  |   1  |      0 0      |  -   |
// |             | Pull-down  |   1  |   0  |               |  0   |
// |             | Pull-up    |   1  |   0  |               |  1   |
// -----------------------------------------------------------------
//
// MODE: 00 - вход, остальные скорость выхода ('01' - 10 МГц, '10' - 2 МГц, '11' - 50 МГц)
//

// CRL -> 0-7 выходы, CRH -> 8-15 выходы
// По 4 бита на выход старшие CNF, младшие MODE (смещение блока = X*4, где X номер выхода с -0 или -8, если больше 7)
// ODR - по одному биту, смещение = номеру выхода

#define TIME_OUT ((uint32_t)(0x7FFFFFFF))
#define HIGH_LEVEL 1
#define LOW_LEVEL 0

void DHT_pin_pull_up(struct DHT_t *dht);
void DHT_pin_output(struct DHT_t *dht, int8_t level);
int8_t DHT_check_pin(struct DHT_t *dht);
uint32_t DHT_check_pulse(struct DHT_t *dht, int8_t level);

struct DHT_t DHT_init(GPIO_TypeDef* port, uint8_t pin, enum DHT_type type) {
    int8_t i = 0;
    struct DHT_t dht;
    for (; i < DHT_DATA; ++i)
        dht.data[i] = 0;
    dht.port = port;
    dht.pin = pin;
    dht.type = type;
    dht.last_read_time = 0;      // ---
    dht.max_cycles = 0x7FFFFFFF; // ---
    dht.result = -1;
    dht.pull_time = 55;
    return dht;
}


int8_t DHT_begin(struct DHT_t *dht) {
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
    DHT_pin_pull_up(dht);
    // установка
    dht->last_read_time = 0;      // ---
    dht->max_cycles = 0x7FFFFFFF; // ~3сек при 72МГц
    return 0;
}

// команда считывания данных, возврат - корректность
int8_t DHT_check_sensor(struct DHT_t *dht) {
    // проверка времени, прошло ли считывание
    // ---
    // обнуление выходов data[i]
    int8_t i;
    for (i = 0; i < DHT_DATA; ++i)
        dht->data[i] = 0;

    // Подтягивание к HIGH, сообщаа о контакте
    DHT_pin_pull_up(dht);
    ms_delay(1);
    // + delay(1); // ожидание 1мс
    
    // Установка низкого уровня (по времени в соответствии с типом датчика)
    DHT_pin_output(dht, LOW_LEVEL);
    switch (dht->type) {
    case DHT22:
    case DHT21:
        ms_delay(2);
        // + delayMicroseconds(1100); // ожидание минимум  ~1мс
        break;
    case DHT11:
    case DHT12:
    default:
        ms_delay(20);
        // + delay(20); // ожидание минимум 18мс
        break;
    }

    uint32_t cycles[80];

    // Завершение сигнала запуска, установкой высокого уровеня на 40 микросекунд.
    DHT_pin_pull_up(dht);
    // Задержка pullTime, стандартно 55, чтобы датчик успел потянуть линию данных на низкое значение.
    mks_delay(dht->pull_time);
    // + delayMicroseconds(pullTime);

    // Ожидание сигналов от DHT

    // Блокировка прерываний, булут мешать корректному считыванию
    // - InterruptLock lock;

    // Сначала идёт низкий сигнал ~80мс, потом высокий ~80 мс
    if (DHT_check_pulse(dht, 0) == TIME_OUT) {
        dht->result = -1;
        return dht->result;
    }
    if (DHT_check_pulse(dht, 1) == TIME_OUT) {
        dht->result = -1;
        return dht->result;
    }

    // DHT отправляет 40 бит информации следующим образом:
    // ~50мс низкий сигнал, потом ~28мс высокий => 0
    // ~50мс низкий сигнал, потом ~70мс высокий => 1
    // В буфер сохраняются длительность каждой волны (=> 80 значений)
    // Их обработка происходит после.
    for (i = 0; i < 80; i += 2) {
        cycles[i] = DHT_check_pulse(dht, 0);
        cycles[i + 1] = DHT_check_pulse(dht, 1);
    }

    // Расшифровка полученных значений 
    for (int i = 0; i < 40; ++i) {
        uint32_t low_cycles = cycles[2 * i];
        uint32_t high_cycles = cycles[2 * i + 1];
        if ((low_cycles == TIME_OUT) || (high_cycles == TIME_OUT)) {
            dht->result = -1;
            return dht->result;
        }
        dht->data[i / 8] <<= 1;
        // Проверка пришедшего сигнала (0 или 1)
        if (high_cycles > low_cycles) {
            dht->data[i / 8] |= 1;
        }
    }

    // Проверка контрольной суммы пришедшего пакета
    uint8_t check = dht->data[0] + dht->data[1] + dht->data[2] + dht->data[3];
    if (dht->data[4] == (check & 0xFF)) {
        dht->result = 0;
    } else {
        dht->result = -1;
    }
    return dht->result;
}

// возврат - корректность
int8_t DHT_check_result(struct DHT_t *dht) {
    return dht->result;
}

// C
float DHT_get_temperature(struct DHT_t *dht) {
    float temp = -273.15;
    // if(dht->result == 0) 
    {
        switch (dht->type) {
        case DHT11:
            temp = dht->data[2];
            if (dht->data[3] & 0x80) {
                temp = -1 - temp;
            }
            temp += (dht->data[3] & 0x0F) * 0.1;
            break;
        case DHT12:
        temp = dht->data[2];
        temp += (dht->data[3] & 0x0F) * 0.1;
        if (dht->data[2] & 0x80) {
            temp *= -1;
        }
        break;
        case DHT22:
        case DHT21:
        temp = ((uint16_t)(dht->data[2] & 0x7F)) << 8 | dht->data[3];
        temp *= 0.1;
        if (dht->data[2] & 0x80) {
            temp *= -1;
        }
        break;
        }
    }
    return temp;
}
// %
float DHT_get_humidity(struct DHT_t *dht) {
    float hum = -100;
    // if(dht->result == 0) 
    {
        switch (dht->type) {
        case DHT11:
        case DHT12:
            hum = dht->data[0] + dht->data[1] * 0.1;
            break;
        case DHT22:
        case DHT21:
            hum = ((uint16_t)dht->data[0]) << 8 | dht->data[1];
            hum *= 0.1;
        break;
        }
    }
    return hum;
}

void DHT_pin_pull_up(struct DHT_t *dht) {
    // настройка пина DHT на вход с подтяжкой вверх (CNF = 10, MODE = 00, ODR = 1)
    if (dht->pin < 8) {
        // обнуляем текущее значение выхода, CNF = MODE = 00
        dht->port->CRL &= ~((GPIO_CRL_MODE0 | GPIO_CRL_CNF0) << (dht->pin * 4));
        // устанавливаем CNF = 10, MODE = 00
        dht->port->CRL |= GPIO_CRL_CNF0_1 << (dht->pin * 4);
    }
    else {
        // обнуляем текущее значение выхода, CNF = MODE = 00
        dht->port->CRH &= ~((GPIO_CRH_MODE8 | GPIO_CRH_CNF8) << ((dht->pin - 8) * 4));
        // устанавливаем CNF = 10, MODE = 00
        dht->port->CRL |= GPIO_CRH_CNF8_1 << ((dht->pin - 8) * 4);
    }
    // устанавливаем ODR = 1
    dht->port->ODR |= GPIO_ODR_ODR0 << dht->pin;
}

void DHT_pin_output(struct DHT_t *dht, int8_t level) {
    // настройка пина DHT на выход со значением level (CNF = 01, MODE = 01, ODR = level)
    if (dht->pin < 8) {
        // обнуляем текущее значение выхода, CNF = MODE = 00
        dht->port->CRL &= ~((GPIO_CRL_MODE0 | GPIO_CRL_CNF0) << (dht->pin * 4));
        // устанавливаем CNF = 01, MODE = 01
        dht->port->CRL |= GPIO_CRL_CNF0_0 << (dht->pin * 4);
        dht->port->CRL |= GPIO_CRL_MODE0_0 << (dht->pin * 4);
    }
    else {
        // обнуляем текущее значение выхода, CNF = MODE = 00
        dht->port->CRH &= ~((GPIO_CRH_MODE8 | GPIO_CRH_CNF8) << ((dht->pin - 8) * 4));
        // устанавливаем CNF = 01, MODE = 01
        dht->port->CRL |= GPIO_CRH_CNF8_0 << ((dht->pin - 8) * 4);
        dht->port->CRL |= GPIO_CRH_MODE8_0 << ((dht->pin - 8) * 4);
    }
    // устанавливаем ODR = level
    if (level == LOW_LEVEL) {
        // = 0
        dht->port->ODR &= ~(GPIO_ODR_ODR0 << dht->pin);
    }
    else {
        // = 1
        dht->port->ODR |= GPIO_ODR_ODR0 << dht->pin;
    }
}

int8_t DHT_check_pin(struct DHT_t *dht) {
    uint32_t level = dht->port->IDR & (GPIO_IDR_IDR0 << dht->pin);
    if (level == LOW_LEVEL) {
        return LOW_LEVEL; // = 0
    }
    return HIGH_LEVEL; // = 1
}

uint32_t DHT_check_pulse(struct DHT_t *dht, int8_t level) {
    uint32_t count = 0;
    while (DHT_check_pin(dht) == level) {
        if (count++ >= dht->max_cycles)
            return TIME_OUT;
    }
    return count;
}