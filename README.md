# blink_bluepill

Firmware bare-metal para a placa **STM32 "Blue Pill" (STM32F103C8T6)**, com build em
CMake + Ninja e toolchain `arm-none-eabi-`. O projeto comeca num blink simples do LED
on-board (PC13) e ja traz integradas as bibliotecas **libopencm3** (drivers de periferico)
e **FreeRTOS** (kernel de tempo real), prontas para uso.

## Hardware

| Item | Valor |
| --- | --- |
| MCU | STM32F103C8T6 (ARM Cortex-M3, sem FPU) |
| FLASH | 64 KB @ `0x08000000` |
| RAM | 20 KB @ `0x20000000` |
| Clock | 72 MHz (HSE 8 MHz + PLL) |
| LED on-board | PC13 (ativo em nivel baixo) |

## Pre-requisitos

- Toolchain `arm-none-eabi-` no `PATH` (vem do STM32CubeCLT / bundle do STM32Cube)
- **CMake** (>= 3.20) e **Ninja**
- **git** (para clonar este repo e as bibliotecas, caso precise refazer)
- Para gravar/depurar: ST-Link + extensao STM32Cube para VS Code (OpenOCD)

> No Windows, use o **Git Bash** para os comandos de shell. O `make` do libopencm3, quando
> necessario, tambem deve rodar no Git Bash.

## Compilar

```sh
cmake --preset Debug          # configura -> build/Debug/
cmake --build --preset Debug
```

Ha dois presets: `Debug` (`-O0 -g3`) e `Release` (`-Os`). A saida fica em `build/<preset>/`:
`blink_bluepill.elf`, mais `.hex` e `.bin` (gerados por um passo objcopy pos-build). Ao
final, e impresso um resumo de uso de memoria (FLASH/RAM).

## Gravar e depurar

Feito pela extensao STM32Cube (ST-Link / OpenOCD). O passo a passo completo (CLI, OpenOCD,
`launch.json` e troubleshooting) esta em [docs/gravar-e-depurar.md](docs/gravar-e-depurar.md).

## Estrutura do projeto

```
blink_bluepill/
├── CMakeLists.txt            # arquivo de build editavel pelo usuario
├── CMakePresets.json         # presets Debug/Release e variaveis do MCU
├── stm32f103x8_flash.ld      # linker script (mapa de memoria)
├── cmake/                    # toolchain + arquivo auto-gerado pelo CubeIDE
├── Inc/
│   └── FreeRTOSConfig.h      # configuracao do kernel FreeRTOS
├── Src/
│   ├── main.c                # blink do PC13 (comentado em nivel iniciante)
│   ├── startup_stm32f103xx.S # startup + vector table
│   ├── syscall.c / sysmem.c  # stubs de libc (nosys)
├── libs/
│   ├── libopencm3/           # biblioteca de drivers (compilada: .a)
│   └── FreeRTOS-Kernel/      # kernel FreeRTOS
└── docs/                     # documentacao (veja abaixo)
```

## Bibliotecas integradas

- **libopencm3** — drivers de periferico, em modo *driver-only* (mantendo o startup e o
  linker do CubeIDE). Procedimento de instalacao e uso em
  [docs/libopencm3.md](docs/libopencm3.md).
- **FreeRTOS** — kernel de tempo real (Cortex-M3, heap_4, 72 MHz). Na branch
  `feature/blink-freertos` o blink roda **sobre o RTOS**, com duas tasks coordenadas por uma
  fila; na `main`, o kernel fica apenas integrado e linkado. Detalhes em
  [docs/freertos.md](docs/freertos.md).

## Documentacao

| Documento | Conteudo |
| --- | --- |
| [docs/gravar-e-depurar.md](docs/gravar-e-depurar.md) | Fluxo de flash/debug (ST-Link, OpenOCD, launch.json) |
| [docs/libopencm3.md](docs/libopencm3.md) | Instalar e usar o libopencm3 |
| [docs/freertos.md](docs/freertos.md) | Instalar e ativar o FreeRTOS |
| [docs/git-workflow.md](docs/git-workflow.md) | Versionamento e gestao de branches |

## Convencoes de build (resumo)

- `CMakeLists.txt` e editavel: adicione fontes em `sources_SRCS` e includes em `include_DIRS`.
- `cmake/vscode_generated.cmake` e **auto-gerado** — nao editar.
- Flags de CPU/FPU sao derivadas das variaveis `CMSIS_D*` em `CMakePresets.json`.
- Avisos estritos: `-Wall -Wextra -Wpedantic`; C11 e C++20 habilitados.

## Licenca

Codigo de startup, syscalls e o `CMakeLists.txt` sao gerados pelo STM32CubeIDE
(Copyright STMicroelectronics, ver cabecalhos). As bibliotecas em `libs/` tem suas proprias
licencas (libopencm3: LGPL-3.0; FreeRTOS: MIT).
