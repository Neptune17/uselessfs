#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>

#include "inode.h"

struct direntry {
  char             name[4096];
  struct inode     *node;
  struct direntry *next;
};

int dir_find(struct inode *dirnode, const char *name, int namelen, struct direntry **entry) {
  struct direntry *ent = (struct direntry *) dirnode->data;

  while(ent != NULL) {
    if(strlen(ent->name) == namelen) {
      if(strncmp(ent->name, name, namelen) == 0) {
        if(entry != NULL) *entry = ent;
        return 1;
      }
    }
    ent = ent->next;
  }

  errno = ENOENT;

  return 0;
}

int dir_add(struct inode *dirnode, struct direntry *entry, int replace, int *added) {
  struct direntry **dir = (struct direntry **) &dirnode->data;
  struct direntry *existing_entry;

  if(dir_find(dirnode, entry->name, strlen(entry->name), &existing_entry)) {
    if(replace) {
      *added = 0;
      existing_entry->node = entry->node;
      return 1;
    } else {
      errno = EEXIST;
      return 0;
    }
  }

  *added = 1;

  if(*dir == NULL) {
    *dir = entry;
    entry->next = NULL;
  } else {
    entry->next = *dir;
    *dir = entry;
  }

  // The entry is now linked in the directory
  entry->node->status.st_nlink++;

  // If the entry is a directory, .. is an implicit hardlink to the parent
  // directory.
  if(S_ISDIR(entry->node->status.st_mode)) {
    dirnode->status.st_nlink++;
  }

  return 1;
}

int dir_add_alloc(struct inode *dirnode, const char *name, struct inode *node, int replace) {
  direntry *entry = new direntry();
  int added;

  if(!entry) {
    errno = ENOMEM;
    return 0;
  }

  strcpy(entry->name, name);
  entry->node = node;

  if(!dir_add(dirnode, entry, replace, &added)) {
    free(entry);
    return 0;
  }

  if(!added) free(entry);

  return 1;
}

int dir_remove(struct inode *dirnode, const char *name) {
  struct direntry **dir = (struct direntry **) &dirnode->data;

  struct direntry *ent = *dir;
  struct direntry **ptr = dir;

  while(ent != NULL) {
    if(strcmp(ent->name, name) == 0) {
      *ptr = ent->next;

      // See dir_add for details
      if(S_ISDIR(ent->node->status.st_mode)) {
        dirnode->status.st_nlink--;
      }

      free(ent);

      return 1;
    }

    ptr = &ent->next;
    ent = ent->next;
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

  // Root directory
  // This also handles paths with trailing slashes (as long as it is a
  // directory) due to recursion.
  if(path[1] == '\0') {
    *retnode = root;
    return 1;
  }

  // Extract name from path
  const char *name = path + 1;
  int namelen = 0;
  const char *name_end = name;
  while(*name_end != '\0' && *name_end != '/') {
    name_end++;
    namelen++;
  }

  // Search directory
  struct direntry *dirent;
  if(!dir_find(root, name, namelen, &dirent)) {
    errno = ENOENT;
    return 0;
  }

  if(*name_end == '\0') {
    // Last node in path
    *retnode = dirent->node;
    return 1;
  } else {
    // Not the last node in path (or a trailing slash)
    return path_to_inode(name_end, dirent->node, retnode);
  }
}