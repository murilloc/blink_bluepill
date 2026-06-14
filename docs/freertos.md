# Instalar e usar o FreeRTOS neste projeto

Este projeto integra o [FreeRTOS-Kernel](https://github.com/FreeRTOS/FreeRTOS-Kernel) ao
firmware do STM32F103 (Blue Pill), que ja usa o libopencm3 como camada de drivers. O kernel
e compilado junto com o projeto pelo CMake (nao ha build separado).

Estado atual: o kernel esta **integrado e em execucao**. Na branch `feature/blink-freertos`,
o `Src/main.c` pisca o LED por **duas tasks** coordenadas por uma **fila (queue)**, sob o
escalonador (sem mais busy-wait); o clock e de 72 MHz. Os detalhes da aplicacao estao na
secao "RTOS em execucao" no fim. (Na branch `main`, o `main.c` ainda usa o blink bare-metal
por busy-wait, com o kernel apenas integrado e linkado.)

## Decisoes de projeto

- **Clock: 72 MHz** (HSE 8 MHz + PLL), via `rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ])`
  do libopencm3, chamado no inicio do `main`. Esse valor **precisa** bater com
  `configCPU_CLOCK_HZ` no `FreeRTOSConfig.h`, senao o tick do RTOS sai errado.
- **Heap:** `heap_4.c` (permite `vPortFree`, com coalescencia). Tamanho em
  `configTOTAL_HEAP_SIZE` (10 KB). Atencao: a Blue Pill tem so **20 KB de RAM** no total.
- **API:** FreeRTOS "cru" (sem a camada CMSIS-RTOS2).
- **Vendoring:** clone simples em `libs/FreeRTOS-Kernel`, versionado como copia fixa
  (o `.git` aninhado foi removido; nao e submodulo).

## Pre-requisitos

Os mesmos do resto do projeto: toolchain `arm-none-eabi-` no PATH, CMake + Ninja. O clone do
kernel usa `git` (com o backend `schannel` no Windows, para evitar erro de certificado SSL).

## Passo 1 — Obter o FreeRTOS-Kernel (uma vez)

A partir da raiz do projeto (a pasta mais interna `blink_bluepill`):

```sh
git -c http.sslBackend=schannel clone --depth 1 \
    https://github.com/FreeRTOS/FreeRTOS-Kernel.git libs/FreeRTOS-Kernel
rm -rf libs/FreeRTOS-Kernel/.git   # versionar como copia fixa (sem submodulo)
```

## Passo 2 — `Inc/FreeRTOSConfig.h`

Arquivo de configuracao do kernel, em [../Inc/FreeRTOSConfig.h](../Inc/FreeRTOSConfig.h)
(`Inc/` ja faz parte de `include_DIRS`). Pontos criticos:

- `configCPU_CLOCK_HZ = 72000000` — casa com o clock configurado no `main`.
- `configTICK_RATE_HZ = 1000` — tick de 1 ms.
- `configTOTAL_HEAP_SIZE = 10*1024` — heap do `heap_4` (cabe nos 20 KB de RAM).
- `configPRIO_BITS = 4` e as prioridades de interrupcao no padrao STM32.
- **Mapeamento dos handlers** para os nomes CMSIS da vector table do Cube:
  ```c
  #define vPortSVCHandler     SVC_Handler
  #define xPortPendSVHandler  PendSV_Handler
  #define xPortSysTickHandler SysTick_Handler
  ```
  Isso funciona porque o startup do Cube
  ([../Src/startup_stm32f103xx.S](../Src/startup_stm32f103xx.S)) declara esses tres handlers
  como **weak**; o `port.c` do FreeRTOS fornece a versao forte, e o linker usa a do kernel.

## Passo 3 — Integracao no `CMakeLists.txt`

Ja aplicado. Em `sources_SRCS` foram adicionados os fontes do kernel:

```
tasks.c  queue.c  list.c  timers.c  event_groups.c  stream_buffer.c
portable/GCC/ARM_CM3/port.c        (port Cortex-M3, GCC)
portable/MemMang/heap_4.c          (gerenciador de heap)
```

E em `include_DIRS`:

```
libs/FreeRTOS-Kernel/include
libs/FreeRTOS-Kernel/portable/GCC/ARM_CM3   (portmacro.h)
```

(`Inc/` ja estava na lista e e onde mora o `FreeRTOSConfig.h`.)

## Passo 4 — Compilar

```sh
cmake --preset Debug
cmake --build --preset Debug
```

## Verificacao da integracao

- Os 8 fontes do kernel geram `.obj` sem erro (em
  `build/Debug/CMakeFiles/blink_bluepill.dir/libs/FreeRTOS-Kernel/`).
- O `.elf` linka sem conflito de `SVC_Handler` / `PendSV_Handler` / `SysTick_Handler`.
- `arm-none-eabi-nm build/Debug/blink_bluepill.elf` mostra `SVC_Handler`, `PendSV_Handler`,
  `SysTick_Handler` como `T` (definicoes fortes do port) e `rcc_clock_setup_pll` (clock 72 MHz).

> **Sobre o tamanho no `.elf`:** o linker usa `-ffunction-sections -Wl,--gc-sections`, entao
> codigo do kernel nao referenciado e descartado. Como os 3 handlers sao referenciados pela
> vector table, boa parte do port entra. Com as tasks ativas, o heap (`ucHeap`, 10 KB) passa
> a ocupar RAM de fato: o build atual usa ~12,7 KB dos 20 KB (61,8%) e ~14 KB de FLASH (21,5%).

## RTOS em execucao (branch `feature/blink-freertos`)

O `Src/main.c` desta branch poe o kernel para rodar de verdade. Para evidenciar a multitarefa
com um unico LED de usuario (PC13), ha **duas tasks** que cooperam por uma **fila**:

- **`vLedTask`** (prioridade 2) — unica dona do PC13. Inverte o LED e dorme com `vTaskDelay`
  (entrega a CPU ao escalonador, ao contrario do busy-wait). A cada volta espia a fila, sem
  bloquear, para adotar um novo ritmo.
- **`vControlTask`** (prioridade 1) — a cada ~3 s envia pela fila o proximo periodo da lista
  `{100, 250, 500, 1000}` ms. Resultado visivel: o pisca acelera/desacelera sozinho, provando
  que ha duas tasks independentes conversando.

Esqueleto (ver o arquivo completo, comentado, em [../Src/main.c](../Src/main.c)):

```c
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

static QueueHandle_t xPeriodQueue;   /* uint32_t: periodo de pisca em ms */

static void vLedTask(void *arg)      /* prioridade 2: dona do LED */
{
    (void)arg;
    uint32_t period_ms = 500;
    for (;;) {
        gpio_toggle(GPIOC, GPIO13);
        xQueueReceive(xPeriodQueue, &period_ms, 0);    /* nao bloqueia */
        vTaskDelay(pdMS_TO_TICKS(period_ms / 2));
    }
}

static void vControlTask(void *arg)  /* prioridade 1: muda o ritmo */
{
    (void)arg;
    static const uint32_t periods[] = { 100, 250, 500, 1000 };
    uint32_t i = 0;
    for (;;) {
        uint32_t next = periods[i];
        xQueueSend(xPeriodQueue, &next, 0);
        i = (i + 1) % (sizeof(periods) / sizeof(periods[0]));
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

int main(void)
{
    rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);
    rcc_periph_clock_enable(RCC_GPIOC);
    gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO13);

    xPeriodQueue = xQueueCreate(1, sizeof(uint32_t));
    xTaskCreate(vLedTask,     "led",  configMINIMAL_STACK_SIZE, NULL, 2, NULL);
    xTaskCreate(vControlTask, "ctrl", configMINIMAL_STACK_SIZE, NULL, 1, NULL);

    vTaskStartScheduler();   /* nunca retorna em operacao normal */
    for (;;);                /* so chega aqui se faltar heap para o escalonador */
}
```

Com o RTOS ativo, `configTOTAL_HEAP_SIZE` (10 KB) ocupa RAM de fato; ajuste se precisar de
mais/menos.
