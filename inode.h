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

#endif