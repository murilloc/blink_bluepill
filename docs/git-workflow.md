# Versionamento e gestao de branches com git

Este projeto e versionado com git. O objetivo deste documento e mostrar, em nivel
iniciante, como guardar varias versoes do firmware (por exemplo, diferentes experimentos
no `Src/main.c`) e alternar entre elas com seguranca.

## Como o repositorio foi inicializado (referencia)

Feito uma unica vez, na raiz do projeto (a pasta mais interna `blink_bluepill`):

```sh
# 1. O libopencm3 foi clonado com o proprio .git dentro. Para versiona-lo como
#    copia fixa (e nao como submodulo), removemos esse .git aninhado:
rm -rf libs/libopencm3/.git

# 2. Cria o repositorio com o branch principal chamado "main":
git init -b main

# 3. Versiona tudo que nao esta no .gitignore (o build/ fica de fora):
git add .
git commit -m "Projeto inicial: blink do PC13 com libopencm3 (modo hibrido)"
```

Decisao adotada: o `libs/libopencm3` **e versionado** (copia fixa, ~17 MB). Assim o
projeto compila em qualquer maquina sem precisar reclonar/recompilar a biblioteca. O
`.gitignore` ignora apenas o diretorio `build/`.

## Conceitos basicos

| Termo | O que e | Para que serve |
| --- | --- | --- |
| **commit** | "Foto" do projeto inteiro num instante, com uma mensagem | Ponto de salvamento. Cada versao do firmware e um commit. |
| **branch** (ramo) | Uma linha de trabalho independente | Manter varios experimentos lado a lado e alternar entre eles. |
| **tag** (etiqueta) | Um nome fixo dado a um commit | Marcar marcos: `exemplo-blink`, `exemplo-pwm`. |
| **HEAD** | Onde voce esta agora (branch/commit atual) | Referencia interna do git. |

O `main` e o ramo principal, onde fica a versao "boa" de referencia.

## Fluxo recomendado: um branch por experimento

Sempre que for tentar uma variacao (piscar mais rapido, ler um botao, usar PWM...),
crie um branch a partir do `main`. Assim o `main` continua intacto e cada experimento
vive isolado.

```sh
# garantir que esta no main e que ele esta limpo
git switch main

# criar e entrar num ramo novo para o experimento
git switch -c blink-rapido

# ...editar Src/main.c, compilar, testar no hardware...

# salvar a versao quando estiver funcionando
git add Src/main.c
git commit -m "Blink mais rapido (delay 100000)"
```

Para voltar a versao de referencia, basta trocar de ramo — o git reescreve os arquivos
no disco para o estado daquele ramo (o `Src/main.c` aberto no editor muda sozinho):

```sh
git switch main          # volta para a versao principal
git switch blink-rapido  # volta para o experimento
```

Criar outro experimento, sempre partindo de uma base limpa:

```sh
git switch main
git switch -c botao-liga-led
```

## Comandos do dia a dia

```sh
git status               # o que mudou e ainda nao foi salvo
git branch               # lista os ramos (o atual tem um *)
git switch <nome>        # troca de ramo
git log --oneline        # historico de versoes (commits), uma por linha
git diff                 # ve as mudancas ainda nao adicionadas
git add Src/main.c       # marca o arquivo para o proximo commit
git commit -m "mensagem" # salva uma nova versao
```

> Importante: o git so deixa trocar de branch se o trabalho atual estiver salvo
> (committado). Se aparecer um aviso ao tentar `git switch`, faca um `git commit`
> antes — ou guarde temporariamente com `git stash` (veja abaixo).

## Guardar trabalho temporariamente (stash)

Quando voce esta no meio de uma alteracao mas precisa trocar de ramo sem fazer commit:

```sh
git stash         # guarda as mudancas e limpa o diretorio
git switch main   # agora pode trocar de ramo
git switch -      # volta para o ramo anterior
git stash pop     # traz as mudancas guardadas de volta
```

## Marcos com tags

Quando um experimento vira um "exemplo pronto" que voce quer reencontrar facil:

```sh
git tag exemplo-blink            # etiqueta o commit atual
git tag                          # lista as tags
git switch --detach exemplo-blink  # "espiar" aquele ponto (sem mexer nos ramos)
git switch main                  # voltar
```

## Espiar uma versao antiga sem perder nada

```sh
git log --oneline                 # descubra o hash da versao desejada
git switch --detach <hash>        # entra naquele commit (modo "destacado")
# ...olhar/compilar...
git switch main                   # volta para o presente
```

Se quiser **trazer de volta** so o `main.c` de uma versao antiga para o ramo atual:

```sh
git checkout <hash> -- Src/main.c   # restaura aquele main.c por cima do atual
```

## Trazer um experimento bom para o `main`

Quando um experimento esta aprovado e voce quer que ele vire a referencia:

```sh
git switch main
git merge blink-rapido            # incorpora o experimento no main
git branch -d blink-rapido        # (opcional) apaga o ramo ja incorporado
```

## (Opcional) Enviar para o GitHub

Para ter backup na nuvem e historico remoto:

```sh
# crie um repositorio vazio no GitHub primeiro, depois:
git remote add origin https://github.com/<usuario>/<repo>.git
git push -u origin main           # envia o main
git push origin blink-rapido      # envia um experimento, se quiser
```
