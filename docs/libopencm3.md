# Instalar e usar o libopencm3 neste projeto

Este projeto integra o [libopencm3](https://github.com/libopencm3/libopencm3) em
**modo híbrido (driver-only)**: o `startup_stm32f103xx.S` e o `stm32f103x8_flash.ld`
gerados pelo STM32CubeIDE continuam sendo usados, e o libopencm3 entra **apenas como
biblioteca de drivers de periférico** (linkamos o `.a`, adicionamos os includes e o
define `STM32F1`). Não usamos a vector table nem o linker script do próprio libopencm3,
o que evita conflito de símbolos com o startup do Cube.

## Pré-requisitos

Na máquina precisam estar disponíveis (já é o caso quando se usa o **Git Bash** com o
STM32CubeCLT instalado):

- `git`, `make`, `python`
- `arm-none-eabi-gcc` no `PATH` (vem do STM32CubeCLT, ex.:
  `C:\ST\STM32CubeCLT_1.21.0\GNU-tools-for-STM32\bin`, ou do bundle do Cube em
  `%CUBE_BUNDLE_PATH%\gnu-tools-for-stm32\<versão>\bin`)

> O `make` do libopencm3 usa shell POSIX — **rode-o a partir do Git Bash**, não do
> PowerShell/cmd.

## Passo 1 — Obter e compilar o libopencm3 (uma vez)

A partir da raiz do projeto (a pasta mais interna `blink_bluepill`):

```sh
git clone --depth 1 https://github.com/libopencm3/libopencm3.git libs/libopencm3
make -C libs/libopencm3 TARGETS=stm32/f1
```

Resultado esperado: `libs/libopencm3/lib/libopencm3_stm32f1.a` e os headers em
`libs/libopencm3/include/`. Compilar apenas `TARGETS=stm32/f1` é bem mais rápido que o
`make` completo.

> **Erro de certificado SSL no clone** (`SSL certificate problem: unable to get local
> issuer certificate`, comum no Windows com antivírus/proxy): use o backend de
> certificados do Windows, sem desabilitar a verificação:
> ```sh
> git -c http.sslBackend=schannel clone --depth 1 \
>     https://github.com/libopencm3/libopencm3.git libs/libopencm3
> ```

A biblioteca é compilada uma única vez; o build normal do projeto apenas linka o `.a`
já pronto. Refaça o `make` somente se atualizar o libopencm3.

## Passo 2 — Integração no `CMakeLists.txt`

Já aplicado neste repositório. As quatro alterações (todas **antes** de
`include("cmake/vscode_generated.cmake")`, que só faz `append`) são:

| Variável        | Valor adicionado                                              |
| --------------- | ------------------------------------------------------------ |
| `include_DIRS`  | `${CMAKE_CURRENT_SOURCE_DIR}/libs/libopencm3/include`        |
| `symbols_SYMB`  | `STM32F1` (obrigatório; sem ele os headers não compilam)     |
| `link_DIRS`     | `${CMAKE_CURRENT_SOURCE_DIR}/libs/libopencm3/lib`           |
| `link_LIBS`     | `opencm3_stm32f1`                                            |

Não é necessário editar `cmake/vscode_generated.cmake` (auto-gerado) nem o startup/linker.

## Passo 3 — Usar a API no código

Exemplo de blink do LED on-board da Blue Pill (PC13, ativo-baixo), em
[../Src/main.c](../Src/main.c):

```c
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

static void delay(volatile uint32_t n) { while (n--) __asm__("nop"); }

int main(void)
{
    rcc_periph_clock_enable(RCC_GPIOC);
    gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ,
                  GPIO_CNF_OUTPUT_PUSHPULL, GPIO13);

    for (;;) {
        gpio_toggle(GPIOC, GPIO13);
        delay(800000);
    }
}
```

## Passo 4 — Compilar o projeto

```sh
cmake --preset Debug
cmake --build --preset Debug
```

Saída em `build/Debug/`: `blink_bluepill.elf` + `.hex` + `.bin`. Gravação/depuração pela
extensão STM32Cube (ST-Link/OpenOCD) — ver [gravar-e-depurar.md](gravar-e-depurar.md).

## Caveats do modo híbrido

- **Interrupções:** a vector table em uso é a do Cube. Os handlers devem usar os **nomes
  CMSIS do Cube** (`SysTick_Handler`, `TIM2_IRQHandler`, ...) e **não** os nomes do
  libopencm3 (`sys_tick_handler`, `tim2_isr`). A vector table do libopencm3 (`vector.c`)
  não é linkada porque o linker script do Cube referencia o `.isr_vector` do startup do Cube.
- **Clock:** o MCU sobe no HSI 8 MHz padrão. Para outra frequência, configure
  explicitamente com `rcc_clock_setup_*` do libopencm3 no início do `main`.
- **Versionamento:** `libs/` não está no `.gitignore` (só `build/` está). Decida se
  versiona o libopencm3 ou o ignora conforme a preferência do repositório.
```
