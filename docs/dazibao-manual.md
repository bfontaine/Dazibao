# Dazibao - User manual

## Command line interface

### Dazibao

#### Usage

```
dazibao [COMMAND] [OPTIONS] [FILE] <dazibao path>
```

note : 
dazibao ne gére pas le long tlv, et le considére comme un tlv standard



Command can be one of theses:

##### `add`

```
dazibao add [OPTIONS and arguments] [FILES] <dazibao path>
```

cette fonction permet d'ajouter plusieurs tlv à la fois, et de différente façons.

option disponible:


* `--type <type>`: force tlvs types (`text`, `jpg`, etc). They must
  be comma-separated if there are multiple TLVs. If the number of specified
  types doesn't match the number of sources or if the option is not present,
  the program will infer their type based on their content.

* `--date`: tout le tlv qui suivent seront inclus dans un tlv date
    exception pour le counpoud pour les fichier qu'il contient, par contre
    on peut creer un tlv date dans un compound.

* `--dazibao`: il a besoin comme arguments un fichier dazibao, qu'il 
    transformera en tlv compound si la taille du fichier le permet

* `--compound`: if present, the TLV will be included in a dated one, using the
  current time.

* `-`: si present prendra un fichier sur l'entrée standard, on peut l'utiliser avec redirection
    ou écrire sur l'entré standard et finir avec ctr-D.

### `compact`

```
dazibao compact <dazibao path>
```

Compact a dazibao, deleted all tlv PAD1 and PADN.

#### `dump`

```
dazibao dump [OPTIONS] <dazibao path>
```

Dump a whole dazibao on the standard output.

Available Options:

* `--depth <n>`: set `<n>` as the maximum depth of the output. Default is 0.
* `--debug`: print `PAD1`s and `PADN`s

#### `extract`

```
dazibao extract <offset> <path to etract file> <dazibao path>
```

permet d'extraire un tlv en donnant ça position dans la le fichier,
pour les compounds crée un dazibao.

#### `rm`

```
dazibao rm <offset> <dazibao path>
```
permet de supprimer un tlv d'un dazibao
