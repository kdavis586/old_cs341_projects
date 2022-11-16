/**
 * mad_mad_access_patterns
 * CS 341 - Fall 2022
 */
#include "tree.h"
#include "utils.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

/*
  Look up a few nodes in the tree and print the info they contain.
  This version uses mmap to access the data.

  ./lookup2 <data_file> <word> [<word> ...]
*/

int main(int argc, char **argv) {
    if (argc < 3) {
      printArgumentUsage();
      return 1;
    }
    char * file_name = argv[1];
    // Open file
    int data_fd = open(file_name, O_RDONLY);
    if (data_fd == -1) {
      // fopen failed
      openFail(file_name);
      close(data_fd);
      return 1;
    }
    struct stat file_info;
    fstat(data_fd, &file_info);

    char * addr = mmap(NULL, (size_t)file_info.st_size, PROT_READ, MAP_PRIVATE, data_fd, 0);

    char start_tag[BINTREE_ROOT_NODE_OFFSET + 1];
    memset(start_tag, 0, BINTREE_ROOT_NODE_OFFSET + 1);
    memcpy(&start_tag, addr, BINTREE_ROOT_NODE_OFFSET);

    if (strcmp(start_tag, BINTREE_HEADER_STRING)) {
      formatFail(file_name);
      close(data_fd);
      return 1;
    }

    // Search for the word information of all the requested words in argv
    size_t target_word_idx = 2;

    char * starting_addr = addr + BINTREE_ROOT_NODE_OFFSET;
    for (; target_word_idx < (size_t)argc; target_word_idx++) {
      // Seek to offset of 4 bytes to start at file data

      char * cur_addr = starting_addr;
      char * target_word = argv[target_word_idx];


      while (1) {
        BinaryTreeNode * cur_node = (BinaryTreeNode *)cur_addr;
        
        int compare_res = strcmp(target_word, cur_node->word);
        if (compare_res == 0) {
          printFound(target_word, cur_node->count, cur_node->price);
          break;
        } else if (compare_res > 0) {
          if (!cur_node->right_child) {
            printNotFound(target_word);
            break;
          } else {
            cur_addr = addr + cur_node->right_child;
          }
        } else {
          if (!cur_node->left_child) {
            printNotFound(target_word);
            break;
          } else {
            cur_addr = addr + cur_node->left_child;
          }
        }
      }
    }

    close(data_fd);
    return 0;
}
