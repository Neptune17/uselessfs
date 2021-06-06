#include <sys/stat.h>

#ifndef _INODE_H
#define _INODE_H

struct inode {
    struct stat status;
    void* data;

    inode(){
        this->data = NULL;
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