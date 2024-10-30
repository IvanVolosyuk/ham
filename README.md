Ham
===
Ham is a command-line tool that applies Hamming error correction codes to a file, adding redundancy that enables recovery from large blocks of corrupted data. With Ham, you can encode and later decode files, ensuring data integrity even with substantial corruption (up to 1 MB per 120 MB).

Features
--------
 * Error Correction: Encodes files with Hamming codes for error correction.
 * Large-Scale Recovery: Recovers files with significant data corruption (up to consecutive 1 MB of corrupted data per 120 MB).
 * If corruption is too large to be corrected the program will likely detect and report it as well.
 * Fast

Build Instructions
------------------
To compile the program, simply run:

```sh
make
```
Usage
-----
To encode a file:

```sh
$ ham <source_file >encoded_file
```
To decode a previously encoded file and fix errors:

```sh
$ ham -d <encoded_file >restored_file
```
