/* @title: Filesystem Globals */
#pragma once

struct vfs_node;

struct fs_globals {
    char *root_partition;
    struct vfs_node *root_node;
};

extern struct fs_globals fs_global;
