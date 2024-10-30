Ham
===
Ham is a command-line tool that applies Hamming error correction codes to a file, adding redundancy that enables recovery from large blocks of corrupted data. With Ham, you can encode and later decode files, ensuring data integrity even with substantial corruption (up to 1 MB per 120 MB).

Inspired by [redupe github project](https://github.com/rescrv/redupe) and [Hamming code (wikipedia)](https://en.wikipedia.org/wiki/Hamming_code).

It works similar to hamming code but instead of individual bits it operates on blocks of 1M bytes doing the same arithmetics on them. Correction logic can detect individual 8-byte blocks within megabyte of data with incorrect checksums and correct those. This way it can restore upto 1 megabyte of corrupted sequential data or a number scattered corruptions. In an unluky case it can fail with just 2 byte of data corrupted in the same offsets of different megabytes of correction code and data. 

For each block of 120M it will add 8M of error codes, thus inflation rate ~6.6% for large files. It designed to work with very large files > 100 MB as it always adds 2 blocks of error correction at the beginning of encoded file, thus resulting encoded file will always be larger than 2MB even for very small inputs resulting in very bad space efficiency in those cases.

Features
--------
 * Error Correction: Encodes files with Hamming codes for error correction.
 * Large-Scale Recovery: Recovers files with significant data corruption (up to consecutive 1 MB of corrupted data per 120 MB).
 * If corruption is too large to be corrected the program will likely detect and report it as well.
 * File size increased only by 6.6% for very large files, 8M codes every 120M of data.
 * Fast

Issues
------
 * Bad space efficiency for small files, resulted files are at least 2M in size.
 * Can fix 1MB of consecutive corruption, but can fail with just 2 bits of corruption on the same offset in different megabytes of data and code.

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
