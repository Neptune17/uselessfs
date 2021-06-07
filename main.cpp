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

#define BLOCKSIZE 4096

struct filesystem {
    struct inode *root;
};

struct filehandler {
	struct inode* node;
	int flags;
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
	
	cout << "readdir" << endl;

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

	cout << "mkdir" << endl;

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
	
	cout << "rmdir" << endl;

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

	cout << "open" << endl;

	struct inode *tarnode;
	if(path_to_inode(path, current_fs.root, &tarnode) == 0) {
		return -errno;
	}

	if(!S_ISREG(tarnode->status.st_mode)) {
		if(S_ISDIR(tarnode->status.st_mode)) {
			return -EISDIR;
		}
	}

	time_t now = time(0);

    tarnode->status.st_atime = now;

	struct filehandler *fh = new filehandler();
	fh->node = tarnode;
	fh->flags = fi->flags;

	fi->fh = (uint64_t) fh;

	tarnode->fd_counter++;

	return 0;
}

static int uselessfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {

	cout << "read" << endl;

	struct filehandler *fh = (struct filehandler *) fi->fh;


	if(!(((fh->flags) & (O_RDWR | O_RDONLY)) | (!((fh->flags) & (O_RDWR | O_WRONLY))))) {
		return -EACCES;
	}

	struct inode *tarnode = fh->node;

	off_t filesize = tarnode->status.st_size;

	if(offset >= filesize) {
		return 0;
	}

	size_t bytes_to_read = min(size, (size_t)filesize - offset);
	memcpy(buf, (char *)tarnode->data + offset, bytes_to_read);

	time_t now = time(0);

	tarnode->status.st_atime = now;

	return bytes_to_read;
}

// static int uselessfs_mknod(const char *path, mode_t mode, dev_t rdev) {

// 	struct inode *newnode;
// 	struct inode *fanode;

// 	if(path_to_inode(safe_dirname(path), current_fs.root, &fanode) == 0) {
// 		return -errno;
// 	}

// 	newnode = new inode();

// 	time_t now = time(0);

//     newnode->status.st_atime = now;
//     newnode->status.st_ctime = now;
//     newnode->status.st_mtime = now;

//     newnode->status.st_mode = mode;
//     newnode->status.st_nlink = 0;
//     newnode->status.st_size = 0;
//     newnode->status.st_blocks = 0;

// 	struct fuse_context *fusectx = fuse_get_context();
// 	newnode->status.st_uid = fusectx->uid;
// 	newnode->status.st_gid = fusectx->gid;

// 	if(add_dirson(fanode, safe_basename(path), newnode) == 0) {
// 		free(newnode);
// 		return -errno;
// 	}

// 	newnode->data = NULL;

// 	return 0;

// }

static int uselessfs_unlink(const char *path) {

	cout << "unlink" << endl;

	struct inode *fanode, *tarnode;

	if(path_to_inode(path, current_fs.root, &tarnode) == 0) {
		return -errno;
	}

	if(S_ISDIR(tarnode->status.st_mode)) {
		return -EISDIR;
	}

	if(path_to_inode(safe_dirname(path), current_fs.root, &fanode) == 0) {
		return -errno;
	}

	if(erase_dirson(fanode, safe_basename(path), strlen(safe_basename(path))) == 0) {
		return -errno;
	}

	if(tarnode->status.st_nlink == 0) {
		if(tarnode->fd_counter == 0) {
			if(tarnode->data){
				free(tarnode->data);
			}
			free(tarnode);
		} else {
			tarnode->delete_label = true;
		}
	}

	return 0;
}

static int uselessfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
	
	cout << "write" << endl;

	struct filehandler *fh = (struct filehandler *) fi->fh;

	if(!((fh->flags) & (O_RDWR | O_WRONLY))) {
		return -EACCES;
	}

	struct inode *tarnode = fh->node;

	blkcnt_t req_blocks = (offset + size + BLOCKSIZE - 1) / BLOCKSIZE;

	if(tarnode->status.st_blocks < req_blocks) {

		void *newdata = malloc(req_blocks * BLOCKSIZE);
		if(!newdata) {
			return -ENOMEM;
		}

		if(tarnode->data != NULL) {
			memcpy(newdata, tarnode->data, tarnode->status.st_size);
			free(tarnode->data);
		}

		tarnode->data = newdata;
		tarnode->status.st_blocks = req_blocks;
	}

	memcpy(((char *) tarnode->data) + offset, buf, size);

	off_t minsize = offset + size;
	if(minsize > tarnode->status.st_size) {
		tarnode->status.st_size = minsize;
	}

	time_t now = time(0);

    tarnode->status.st_atime = now;
    tarnode->status.st_mtime = now;

	return size;
}

static int uselessfs_release(const char *path, struct fuse_file_info *fi) {

	cout << "release" << endl;

	struct filehandler *fh = (struct filehandler *) fi->fh;

	fh->node->fd_counter -= 1;

	if(fh->node->fd_counter == 0 && fh->node->delete_label == true){
		if(fh->node->data != NULL){
			free(fh->node->data);
		}
		free(fh->node);
	}

	free(fh);

	return 0;
}

static int uselessfs_getattr(const char *path, struct stat *stbuf) {
	
	cout << "getattr" << endl;

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

	if(S_ISDIR(_node->status.st_mode)) {
		stbuf->st_nlink++;
	}

	return 0;
}

int uselessfs_create(const char *path, mode_t mode, struct fuse_file_info * fi){

	cout << "create" << endl;

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

	struct filehandler *fh = new filehandler();
	fh->node = newnode;
	fh->flags = fi->flags;

	fi->fh = (uint64_t) fh;

	newnode->fd_counter++;

	return 0;
}

static int uselessfs_utimens(const char *path, const struct timespec ts[2]) {
	
	cout << "utimens" << endl;
	
	struct inode *tarnode;
	if(path_to_inode(path, current_fs.root, &tarnode) == 0) {
		return -errno;
	}

	tarnode->status.st_atime = ts[0].tv_sec;
	tarnode->status.st_mtime = ts[1].tv_sec;

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
	//.mknod		= 	uselessfs_mknod,
	.mkdir      = 	uselessfs_mkdir,
	.unlink     =	uselessfs_unlink,
	.rmdir		= 	uselessfs_rmdir,
	.open		= 	uselessfs_open,
	.read		= 	uselessfs_read,
	.write		=	uselessfs_write,
	.release	= 	uselessfs_release,
	.readdir	= 	uselessfs_readdir,
	.create		=	uselessfs_create,
	.utimens	=	uselessfs_utimens,
};

int main(int argc, char *argv[]){

    init_root_inode();

    return fuse_main(argc, argv,  &uselessfs_oper, NULL);
}