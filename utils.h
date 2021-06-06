#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>

#include "inode.h"

int find_dirson(struct inode *dirnode, const char *name, unsigned int namelen, struct dirson **retdirson) {
	
	// find correct son under dir inode by compare name
	
	struct dirson *soniter = (struct dirson *) dirnode->data;

	while(soniter != NULL) {
		if(strlen(soniter->name) == namelen) {
			if(strncmp(soniter->name, name, namelen) == 0) {
				if(retdirson != NULL) *retdirson = soniter;
				return 1;
			}
		}
		soniter = soniter->next;
	}

	errno = ENOENT;

	return 0;
}

int path_to_inode(const char *path, struct inode *root, struct inode **retnode){
	
	if(!S_ISDIR(root->status.st_mode)) {
		errno = ENOTDIR;
		return 0;
	}

	if(path[0] != '/') {
		errno = EINVAL;
		return 0;
	}

	if(path[1] == '\0') {
		*retnode = root;
		return 1;
	}

	const char *name = path + 1;
	int namelen = 0;
	const char *name_end = name;
	while(*name_end != '\0' && *name_end != '/') {
		name_end++;
		namelen++;
	}

	struct dirson *dirent;
	if(find_dirson(root, name, namelen, &dirent) == 0) {
		errno = ENOENT;
		return 0;
	}

	if(*name_end == '\0') {
		*retnode = dirent->node;
		return 1;
	} else {
		return path_to_inode(name_end, dirent->node, retnode);
	}
}