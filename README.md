ham
======

This package provides 'ham' command which adds hamming error correction codes
to standard input and produces encoded redundant file. The file have to be
decoded using 'ham -d' command which can recover large blocks of corrupted data
upto 1M every 120M.

BUILD
-----

```make```

USAGE
-----

```
$ ham <source_file >encoded_file
$ ham -d <encoded_file >restored_file
```
