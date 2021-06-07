#include <sys/stat.h>

#ifndef _INODE_H
#define _INODE_H

struct inode {
    struct stat status;
    void* data;

    int fd_counter;
    bool delete_label;

    inode(){
        this->data = NULL;
        this->fd_counter = 0;
        this->delete_label = false;
    }
};

// for dir inode, data contains the start pointer of its sons pointer list
// struct dirson is pointer list node

struct dirson {
    char name[4096];
    struct inode *node;
    struct dirson *next;
};

#endif