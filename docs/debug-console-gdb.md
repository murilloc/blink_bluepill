# Debug Console — comandos GDB (Blue Pill / STM32F103)

Referência rápida dos comandos úteis na **Debug Console** do VS Code durante uma
sessão de debug com a extensão STM32Cube (adaptador `stlinkgdbtarget`, GDB
`arm-none-eabi-gdb` do bundle). Complementa o fluxo de gravação/debug em
[gravar-e-depurar.md](gravar-e-depurar.md).

## Como a Debug Console interpreta o que você digita

A Debug Console tem **dois modos** de entrada:

| Entrada | Interpretação | Exemplo |
|---|---|---|
| **sem** prefixo | avalia como **expressão** (no frame atual) | `uxTask`, `2 + 3`, `*pxPort` |
| com prefixo `>` | envia o comando **cru ao GDB** | `>show version`, `>info registers` |

Por isso:

- **Comandos do GDB exigem o `>`.** Digitar `show version` sem o prefixo dá o erro
  `Evaluation of expression without frameId is not supported`, porque o VS Code tenta
  *avaliar uma expressão* e — se a CPU está rodando — não há frame de pilha selecionado.
  Com `>`, o GDB executa o comando independentemente de frame e o erro some.
- **Avaliar expressões** (nome de variável, `*ponteiro`, etc.) só funciona com o
  programa **parado (halted)** num breakpoint, pois aí existe um frame com escopo.

Confirmação de que o console está vivo (este ambiente):

```
>show version
GNU gdb (GNU Tools for STM32 14.3.rel1.20251027-0700) 15.2.90.20241229-git
...
This GDB was configured as "--host=x86_64-w64-mingw32 --target=arm-none-eabi".
```

---

## Comandos GDB (use sempre com `>`)

### Sessão / informação
| Comando | O que faz |
|---|---|
| `>show version` | versão e configuração do GDB |
| `>help` | lista de categorias de ajuda |
| `>help <cmd>` | ajuda de um comando específico (ex.: `>help break`) |
| `>info all-registers` | todos os registradores do core |
| `>info registers` | registradores de uso geral (r0–r15, xPSR) |

### Execução
| Comando | O que faz |
|---|---|
| `>continue` / `>c` | continua a execução (= `F5`) |
| `>next` / `>n` | step over (uma linha, sem entrar em funções) |
| `>step` / `>s` | step into (entra na função) |
| `>finish` | roda até retornar da função atual |
| `>interrupt` | pausa/halt o alvo (= botão Pause) |

> Os botões da barra de debug (`F5`/`F10`/`F11`) fazem o mesmo e são mais práticos.
> O console brilha mesmo é para o que **não tem botão** (abaixo).

### Breakpoints / watchpoints
| Comando | O que faz |
|---|---|
| `>break main` | breakpoint na função `main` |
| `>break main.c:42` | breakpoint no arquivo/linha |
| `>tbreak <local>` | breakpoint temporário (some após o 1º hit) |
| `>info breakpoints` | lista breakpoints |
| `>delete <n>` | remove o breakpoint nº `n` |
| `>watch uxCounter` | para quando a variável **muda** de valor |
| `>rwatch uxCounter` | para quando a variável é **lida** |

### Inspeção de memória / variáveis
| Comando | O que faz |
|---|---|
| `>print uxCounter` / `>p uxCounter` | imprime valor (com `/x` para hex: `>p/x var`) |
| `>display uxCounter` | reimprime a cada parada |
| `>ptype <var>` | mostra o tipo/struct |
| `>x/8xw 0x20000000` | dump de memória: 8 words em hex a partir do endereço |
| `>set var uxCounter = 0` | altera o valor de uma variável |
| `>backtrace` / `>bt` | pilha de chamadas do frame atual |
| `>frame <n>` / `>f <n>` | seleciona um frame da pilha |

> Para **variáveis em escopo**, é mais simples digitá-las **sem `>`** (avaliação direta)
> ou usar o painel **Variables**/**Watch** do VS Code.

---

## Comandos do servidor (`>monitor ...`)

`monitor` repassa o comando ao **ST-Link GDB Server** (camada OpenOCD-like). Úteis:

| Comando | O que faz |
|---|---|
| `>monitor reset` | reseta o alvo |
| `>monitor halt` | para a CPU |
| `>monitor reg` | registradores via servidor |

> A disponibilidade exata depende do servidor; se um `monitor` não responder, use o
> equivalente puro do GDB (ex.: `>info registers` em vez de `>monitor reg`).

---

## Periféricos (RCC, GPIO, ...) — use o viewer, não o console

Registradores de periféricos (ex.: `GPIOC->ODR`, `RCC->APB2ENR`) são melhor lidos pelo
**viewer de periféricos** habilitado pelo `svdPath` no `launch.json` (extensão
`stm32cube-ide-registers`), que decodifica os campos. Pelo console dá para ler o endereço
cru, ex. o `ODR` do GPIOC (`0x4001100C`):

```
>x/1xw 0x4001100C
```

---

## Dicas específicas de FreeRTOS

Com o kernel rodando (duas tasks + fila), o que costuma ajudar na hora de parar:

- `>bt` logo após um Pause mostra em qual task/contexto a CPU estava.
- `>print pxCurrentTCB` → TCB da task atual; `>print *pxCurrentTCB` expande a struct
  (nome da task em `pcTaskName` se `configMAX_TASK_NAME_LEN` permitir).
- `>print uxCurrentNumberOfTasks` → quantas tasks existem.
- Breakpoint dentro de uma task (`>break <func_da_task>`) para inspecionar a coordenação
  pela fila sem parar o scheduler manualmente.

> Para visão de tasks/filas em árvore, o ideal é um plugin RTOS-aware do GDB; sem ele,
> os comandos acima cobrem o essencial.

---

## Erro comum

**`Evaluation of expression without frameId is not supported`**
→ Você mandou um comando GDB **sem o `>`** (ou tentou avaliar uma expressão com a CPU
rodando). Solução: use `>` para comandos GDB; para avaliar variáveis, pare num breakpoint
primeiro.
