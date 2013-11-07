# Dazibao API

`typedef int dz_t;`

- - - - - - - - - -

### `int dz_create(dz_t *daz_buf,  char *path)`

Create a new dazibao in a file given by `path`, and fill `daz_buf` accordingly. If this file already exist, the function doesn't override it and return an error status.

#### Return value

* 0 on success
* -1 on error

- - - - - - - - - -

### `int dz_open(dz_t* d,  char* path,  int flags)`

Open a dazibao. `flags` flags are passed to the open(2) call used to open the file.

#### Return value

* 0 on success
* -1 on error

- - - - - - - - - -

### `int dz_close(dz_t* d)`

Close a dazibao.

### Return value

0 on success

- - - - - - - - - -

### `int dz_read_tlv(dz_t* d, char *tlv, off_t offset)`

Fill all the fields of `buf` with the TLV located at `offset` offset in the dazibao `d`.

#### Return value

* 0 on success
* -1 on error

- - - - - - - - - -

### `off_t dz_next_tlv(dz_t* d, char *tlv)`

Fill the type and length attributes of `buf` with the TLV located at the current offset in the dazibao `d`. The function returns the offset of this TLV on success, EOD if the end of file has been reached, and -1 on error.

- - - - - - - - - -

### `int dz_tlv_at(dz_t* d, char *tlv, off_t offset)`

Fill the type and length attributes of `buf` with the TLV located at offset `offset` in the dazibao. The current offset is not modified.
 
#### Return value

* 0 on success
* -1 on error

- - - - - - - - - -

### `int dz_write_tlv_at(dz_t *d, char *tlv, off_t offset)`

- - - - - - - - - -

### `int dz_add_tlv(dz_t* d, char *tlv)`

Add a TLV at the end of a dazibao. If the dazibao ends with a sequence of pad1/padN's, the TLV `src` overrides the beginning of the sequence, and the file is truncated if the TLV is smaller than the total size of the sequence.

#### Return value

- - - - - - - - - -

### `off_t dz_pad_serie_start(dz_t* d, off_t offset)`

Look for the beggining of an unbroken pad1/padN serie leading to `offset`.

#### Return value
* Offset of the begging of this serie on search succes
* `offset` if search was unsuccessful

- - - - - - - - - -

### `off_t dz_pad_serie_end(dz_t* d, off_t offset)`

Skip tlv at `offset`, and look for the end of an unbroken pad1/padN serie starting after the skipped tlv.

#### Return value
* Offset of the end of this serie on search succes
* `offset` if search was unsuccessful

- - - - - - - - - -

### `int dz_rm_tlv(dz_t* d, off_t offset)`

Remove the TLV at 'offset' from a dazibao.

#### Return value

- - - - - - - - - -

### `int dz_do_empty(dz_t *d, off_t start, off_t length)`

Empty a part of a dazibao, starting at `start`, and of length `length`. The part is filled with padN's and pad1's.

#### Return value

- - - - - - - - - -

### `int dz_compact(dz_t *d)`

Compact a Dazibao file. The file must have been opened in read/write mode, and the Dazibao is NOT closed by the function. Also, the dazibao offset is NOT preserved.
 
### Return value

* number of bytes saved by the compacting operation on success
* -1 if an error occured.

- - - - - - - - - -

### `int dz_dump(dz_t *daz_buf)`

Print tlvs contained in `daz_buf` on standard output.

- - - - - - - - - -
