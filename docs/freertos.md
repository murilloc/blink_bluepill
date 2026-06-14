# Instalar e usar o FreeRTOS neste projeto

Este projeto integra o [FreeRTOS-Kernel](https://github.com/FreeRTOS/FreeRTOS-Kernel) ao
firmware do STM32F103 (Blue Pill), que ja usa o libopencm3 como camada de drivers. O kernel
e compilado junto com o projeto pelo CMake (nao ha build separado).

Estado atual: o kernel esta **integrado, compilando e linkado**, mas **nenhuma task e criada
e o escalonador nao e iniciado** ("so integrar o kernel"). O `Src/main.c` segue piscando o
LED por busy-wait; o clock foi elevado para 72 MHz. Para por o RTOS para rodar de fato, veja
a secao "Ativar o RTOS" no fim.

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
> vector table, boa parte do port entra; ja o heap (`ucHeap`, 10 KB) so passa a ocupar RAM
> quando `pvPortMalloc`/`xTaskCreate` forem usados. Por isso, enquanto nao ha tasks, a RAM
> usada continua baixa.

## Ativar o RTOS (proximo passo)

Para por o kernel para rodar, crie ao menos uma task e inicie o escalonador no `main`. O
mapeamento de handlers e o clock de 72 MHz ja estao prontos.

```c
#include "FreeRTOS.h"
#include "task.h"
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

static void blink_task(void *arg)
{
    (void)arg;
    for (;;) {
        gpio_toggle(GPIOC, GPIO13);
        vTaskDelay(pdMS_TO_TICKS(500));   /* 500 ms, agendado pelo RTOS */
    }
}

int main(void)
{
    rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);
    rcc_periph_clock_enable(RCC_GPIOC);
    gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO13);

    xTaskCreate(blink_task, "blink", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    vTaskStartScheduler();   /* nunca retorna */
    for (;;);                /* so chega aqui se faltar heap para o escalonador */
}
```

Ao ativar, lembre que `configTOTAL_HEAP_SIZE` (10 KB) passa a ocupar RAM de fato; ajuste se
precisar de mais/menos.
