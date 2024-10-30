/**
 * ham: encode with hamming codes and decode correcting errors
 * Copyright (C) 2024  Ivan Volosyuk
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <https://www.gnu.org/licenses/>.
 */

#include <cassert>
#include <cstring>
#include <string>
#include <vector>
#include <unistd.h>
#include <cstdint>
#include <fcntl.h>

size_t read_fully(int fd, char *buffer, size_t sz) {
  size_t offset = 0;
  while (offset < sz) {
    int r = read(fd, buffer + offset, sz - offset);
    if (r < -1) {
      perror("read");
      break;
    }
    if (r == 0) {
      // EOF
      break;
    }
    //fprintf(stderr, "Read: %d bytes\n", r);
    offset += r;
  }
  return offset;
}

size_t write_fully(int fd, char *buffer, size_t sz) {
  size_t offset = 0;
  while (offset < sz) {
    int r = write(fd, buffer + offset, sz - offset);
    if (r < -1) {
      perror("write");
      exit(1);
    }
    if (r == 0) {
      exit(1);
    }
    offset += r;
  }
  return offset;
}

bool isCode(int i) {
  return (i & (i - 1)) == 0;
}

class Encoder {
public:
  Encoder(int in_fd, int out_fd, size_t block_size, int length)
    : in_fd_(in_fd), out_fd_(out_fd), block_size_(block_size), length_(length) {
    // Force block size to be diviable by 8 for efficiency.
    assert((block_size & 7) == 0);
    buffer_.resize(length);

    for (size_t i = 1; i < length; i++) {
      buffer_[i].resize(block_size);
    }
  }

  bool encode() {
    int i;
    size_t last_block_size;

    for (i = 1; i < length_; i++) {
      if (isCode(i)) {
        //fprintf(stderr, "Code added [%d]\n", i);
        // Code point, fill with zeros for now.
        memset(buffer_[i].data(), 0, block_size_);
        continue;
      }
      //fprintf(stderr, "Reading data [%d]\n", i);
      last_block_size = read_fully(in_fd_, buffer_[i].data(), block_size_);
      if (last_block_size != block_size_) {
        i++;
        break;
      }
    }
    if (last_block_size == 0) {
      //fprintf(stderr, "Last block size is zero\n");
      last_block_size = block_size_;
      i--;
      // Those codes are not needed
      while (isCode(i - 1) && i > 1) i--;
    }

    if (i == 1) return false;

    if (last_block_size != block_size_) {
      // Zero out bytes after the end of last block.
      memset(buffer_[i-1].data() + last_block_size,
             0, block_size_ - last_block_size);
    }

    int nblocks = i;
    //fprintf(stderr, "nblocks %d\n", nblocks);

    // Generate hamming codes
    for (int b = 1; b < nblocks; b++) {
      if (isCode(b)) {
        // Code point, skip
        continue;
      }
      for (int shift = 1; shift < nblocks; shift <<= 1) {
        if (!(b & shift)) continue;
        //fprintf(stderr, "[%d] -> [%d]\n", b, shift);
        uint64_t* code = (uint64_t*) buffer_[shift].data();
        uint64_t* data = (uint64_t*) buffer_[b].data();
        size_t qwords = block_size_ / sizeof(uint64_t);
        for (size_t i = 0; i < qwords; i++) {
          code[i] ^= data[i];
        }
      }
    }

    // Output results.
    for (int b = 1; b < nblocks - 1; b++) {
      //fprintf(stderr, "Write[%d]: %ld bytes\n", b, block_size_);
      write_fully(out_fd_, buffer_[b].data(), block_size_);
    }
    //fprintf(stderr, "Write last[%d]: %ld bytes\n", nblocks - 1, last_block_size);
    write_fully(out_fd_, buffer_[nblocks - 1].data(), last_block_size);
    return last_block_size == block_size_;
  }

private:
  int in_fd_;
  int out_fd_;
  size_t block_size_;
  int length_;
  std::vector<std::string> buffer_;

};

class Decoder {
public:
  Decoder(int in_fd, int out_fd, size_t block_size, int length)
    : in_fd_(in_fd), out_fd_(out_fd), block_size_(block_size), length_(length) {
    // Force block size to be diviable by 8 for efficiency.
    assert((block_size & 7) == 0);
    buffer_.resize(length);

    for (size_t i = 1; i < length; i++) {
      buffer_[i].resize(block_size);
    }
  }

  bool decode() {
    size_t nqwords = block_size_ / sizeof(uint64_t);
    int i;
    size_t last_block_size;

    for (i = 1; i < length_; i++) {
      //fprintf(stderr, "Reading data [%d]\n", i);
      last_block_size = read_fully(in_fd_, buffer_[i].data(), block_size_);
      if (last_block_size != block_size_) {
        i++;
        break;
      }
    }

    if (last_block_size != block_size_) {
      // Zero out bytes after the end of last block.
      memset(buffer_[i-1].data() + last_block_size,
             0, block_size_ - last_block_size);
    }

    int nblocks = i;
    //fprintf(stderr, "nblocks %d\n", nblocks);

    // Clear hamming codes
    for (int b = 1; b < nblocks; b++) {
      if (isCode(b)) {
        // Code point, skip
        continue;
      }
      for (int shift = 1; shift < nblocks; shift <<= 1) {
        if (!(b & shift)) continue;
        //fprintf(stderr, "[%d] -> [%d]\n", b, shift);
        uint64_t* code = (uint64_t*) buffer_[shift].data();
        uint64_t* data = (uint64_t*) buffer_[b].data();
        for (size_t i = 0; i < nqwords; i++) {
          code[i] ^= data[i];
        }
      }
    }

    // Fix errors
    for (int b = 1; b < nblocks; b <<= 1) {
      uint64_t* code = (uint64_t*) buffer_[b].data();
      for (size_t i = 0; i < nqwords; i++) {
        if (code[i] == 0) continue;
        //fprintf(stderr, "Found traces of error at [%d:%ld]: %lx\n", b, i, code[i]);
        recover_qword(i, nblocks, code[i]);
      }
    }

    // Output results.
    for (int b = 1; b < nblocks - 1; b++) {
      if (isCode(b)) continue;
      //fprintf(stderr, "Write[%d]: %ld bytes\n", b, block_size_);
      write_fully(out_fd_, buffer_[b].data(), block_size_);
    }
    //fprintf(stderr, "Write last[%d]: %ld bytes\n", nblocks - 1, last_block_size);
    write_fully(out_fd_, buffer_[nblocks - 1].data(), last_block_size);
    stream_offset += (nblocks - 2) * block_size_ + last_block_size;
    return last_block_size == block_size_;
  }

  bool recover_qword(size_t offset, int nblocks, uint64_t qword) {
    int index = 0;
    bool unrecoverable = false;

    for (int b = 1; b < nblocks; b <<= 1) {
      uint64_t* data = getbuf(b);
      if (data[offset] == qword) {
        index |= b;
      } else if (data[offset] != 0) {
        unrecoverable = true;
      }
      data[offset] = 0;
    }

    if (unrecoverable) {
      // May be possible to restore individual bytes or bits
      fprintf(stderr,
              "Double (uncorrectable) error detected at block %ld, offset %ld\n",
              stream_offset, offset);
      total_unrecoverable_errors++;
      return false;
    }

    uint64_t* data = (uint64_t*) buffer_[index].data();
    data[offset] ^= qword;
    total_recoverable_errors++;
    // Data corrupted at index
    uint64_t stream_pos = stream_offset + (index-1) * block_size_ +
      offset * sizeof(uint64_t);
    if (isCode(index)) {
      fprintf(stderr, "Ignored corruption at recovery codes [index %d, offset %ld]\n",
              index, stream_pos);
    } else {
      fprintf(stderr, "Corrected corrupted qword [index %d, offset %ld]\n",
              index, stream_pos);
    }
    return true;
  }

  uint64_t* getbuf(int b) { return (uint64_t*) buffer_[b].data(); }

private:
  int in_fd_;
  int out_fd_;
  size_t block_size_;
  int length_;
  std::vector<std::string> buffer_;
  uint64_t total_recoverable_errors = 0;
  uint64_t total_unrecoverable_errors = 0;
  uint64_t stream_offset = 0;
};

size_t BLOCK_SIZE = 1024 * 1024;
int LENGTH = 128;

const char* HELP_STR =
  R"(The program works similar to 'cat', but encodes input adding redundancy codes to its output.
With -d parameter it decodes the redundant file and corrects errors.
It can restore upto %ld corrupted consecutive bytes every %lld bytes
)";

int main(int argc, char **argv) {
  int stride = 0;
  for (int i = 0; i < LENGTH; i++) {
    if (isCode(i)) continue;
    stride++;
  }

  if (argc > 1) {
    if (std::string("-h") == argv[1]) {
      fprintf(stderr, HELP_STR, BLOCK_SIZE, BLOCK_SIZE * stride);
      return 0;
    }
    if (std::string("-d") == std::string(argv[1])) {
      Decoder e(STDIN_FILENO, STDOUT_FILENO, BLOCK_SIZE, LENGTH);
      while(e.decode()) {}
      exit(0);
    }
  }

  Encoder e(STDIN_FILENO, STDOUT_FILENO, BLOCK_SIZE, LENGTH);
  while(e.encode()) {}
}
