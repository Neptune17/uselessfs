#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/time.h>

#include "inode.h"
#include "utils.h"

struct filesystem {
    struct inode *root;
};

struct filesystem current_fs;

static int uselessfs_getattr(const char *path, struct stat *stbuf) {
  struct inode *_node;
  if(path_to_inode(path, current_fs.root, &_node) == 0) {
    return -errno;
  }

  stbuf->st_mode   = _node->status.st_mode;
  stbuf->st_nlink  = _node->status.st_nlink;
  stbuf->st_size   = _node->status.st_size;
  stbuf->st_blocks = _node->status.st_blocks;
  stbuf->st_uid    = _node->status.st_uid;
  stbuf->st_gid    = _node->status.st_gid;
  stbuf->st_mtime  = _node->status.st_mtime;
  stbuf->st_atime  = _node->status.st_atime;
  stbuf->st_ctime  = _node->status.st_ctime;

  return 0;
}

void init_root_inode(){
    
    inode *root = new inode();

    time_t now = time(0);
    root->status.st_atime = now;
    root->status.st_ctime = now;
    root->status.st_mtime = now;

    root->status.st_uid = getuid();
    root->status.st_gid = getgid(); 
    root->status.st_mode = S_IFDIR | 0755;
    root->status.st_nlink = 0;
    root->status.st_size = 0;
    root->status.st_blocks = 0;

    root->data = NULL;

    current_fs.root = root;
}

static struct fuse_operations uselessfs_oper = {
  .getattr      = uselessfs_getattr
};

int main(int argc, char *argv[]){

    init_root_inode();

    return fuse_main(argc, argv,  &uselessfs_oper, NULL);
}