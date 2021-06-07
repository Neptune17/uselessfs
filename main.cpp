#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <errno.h>

#include <sys/time.h>

#include "inode.h"
#include "utils.h"

#include <iostream>
using namespace std;

struct filesystem {
    struct inode *root;
};

struct filesystem current_fs;

char * safe_dirname(const char *msg) {
	char *buf = strdup(msg);
	char *dir = dirname(buf);
	char *res = strdup(dir);
	free(buf);
	return res;
}

char * safe_basename(const char *msg) {
	char *buf = strdup(msg);
	char *nam = basename(buf);
	char *res = strdup(nam);
	free(buf);
	return res;
}

static int uselessfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
	
	struct inode *tarnode;
	if(path_to_inode(path, current_fs.root, &tarnode) == 0) {
		return -errno;
	}

	if(!S_ISDIR(tarnode->status.st_mode)) {
		return -ENOTDIR;
	}

	filler(buf, ".",  &tarnode->status, 0);
	if(tarnode == current_fs.root) {
		filler(buf, "..", NULL, 0);
	} else {
		struct inode *fanode;
		path_to_inode(safe_dirname(path), current_fs.root, &fanode);
		filler(buf, "..", &fanode->status, 0);
	}

	struct dirson *soniter = (struct dirson *) tarnode->data;
	while(soniter != NULL) {
		if(filler(buf, soniter->name, &soniter->node->status, 0))
			break;
		soniter = soniter->next;
	}

	return 0;
}

static int uselessfs_mkdir(const char *path, mode_t mode) {

	struct inode *newnode;
	struct inode *fanode;

	if(path_to_inode(safe_dirname(path), current_fs.root, &fanode) == 0) {
		return -errno;
	}

	newnode = new inode();

	time_t now = time(0);

    newnode->status.st_atime = now;
    newnode->status.st_ctime = now;
    newnode->status.st_mtime = now;

    newnode->status.st_mode = S_IFDIR | mode;
    newnode->status.st_nlink = 0;
    newnode->status.st_size = 0;
    newnode->status.st_blocks = 0;

	struct fuse_context *fusectx = fuse_get_context();
	newnode->status.st_uid = fusectx->uid;
	newnode->status.st_gid = fusectx->gid;

	if(add_dirson(fanode, safe_basename(path), newnode) == 0) {
		free(newnode);
		return -errno;
	}

	newnode->data = NULL;

	return 0;

}

static int uselessfs_rmdir(const char *path) {
	
	struct inode *tarnode;
	struct inode *fanode;

	if(path_to_inode(path, current_fs.root, &tarnode) == 0) {
		return -errno;
	}

	if(!S_ISDIR(tarnode->status.st_mode)) {
		return -ENOTDIR;
	}

	if(tarnode->data != NULL) {
		return -ENOTEMPTY;
	}

	path_to_inode(safe_dirname(path), current_fs.root, &fanode);

	int ret = erase_dirson(fanode, safe_basename(path), strlen(safe_basename(path)));

	if(ret == 1){
		return 0;
	}
	else{
		return -errno;
	}
}

static int uselessfs_open(const char *path, struct fuse_file_info *fi) {

}

static int uselessfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {

}

static int uselessfs_mknod(const char *path, mode_t mode, dev_t rdev) {

	struct inode *newnode;
	struct inode *fanode;

	if(path_to_inode(safe_dirname(path), current_fs.root, &fanode) == 0) {
		return -errno;
	}

	newnode = new inode();

	time_t now = time(0);

    newnode->status.st_atime = now;
    newnode->status.st_ctime = now;
    newnode->status.st_mtime = now;

    newnode->status.st_mode = mode;
    newnode->status.st_nlink = 0;
    newnode->status.st_size = 0;
    newnode->status.st_blocks = 0;

	struct fuse_context *fusectx = fuse_get_context();
	newnode->status.st_uid = fusectx->uid;
	newnode->status.st_gid = fusectx->gid;

	if(add_dirson(fanode, safe_basename(path), newnode) == 0) {
		free(newnode);
		return -errno;
	}

	newnode->data = NULL;

	return 0;

}

static int uselessfs_unlink(const char *path) {

}

static int uselessfs_rename(const char *from, const char *to) {

}

static int uselessfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi){

}

static int useless_release(const char *path, struct fuse_file_info *fi) {

}

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
  	.getattr	= 	uselessfs_getattr,
	.mknod		= 	uselessfs_mknod,
	.mkdir      = 	uselessfs_mkdir,
	.rmdir		= 	uselessfs_rmdir,
	.readdir	= 	uselessfs_readdir,
};

int main(int argc, char *argv[]){

    init_root_inode();

    return fuse_main(argc, argv,  &uselessfs_oper, NULL);
}