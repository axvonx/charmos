#include <block/block.h>
#include <err.h>
#include <fs/detect.h>
#include <fs/global.h>
#include <mem/alloc.h>
#include <scaled_param.h>

#include "fs/ext2/tests/test_internal.h"

TEST_GROUP_DEFINE(ext2, .intensity_desc = {
                            .curve = SCALE_PIECEWISE_LOG,
                            .unit = "ops",
                        });

#define EXT2_ROOT struct vfs_node *root = fs_global.root_node

static void flush(void) {
    struct ext2_fs *fs = fs_global.root_node->fs_data;
    struct block_device *d = fs->drive;

    bio_sched_dispatch_all(d);
}

TEST_DEFINE_INTEGRATION(ext2, stat, TEST_INTENSITY(1, 1, 16),
                        .required_fs = FS_EXT2) {
    EXT2_ROOT;
    TEST_ASSERT(!ERR_IS_FATAL(
        root->ops->create(root, "ext2_stat_test", VFS_MODE_FILE)));

    struct vfs_node *node;
    struct vfs_dirent out;

    TEST_ASSERT(
        !ERR_IS_FATAL(root->ops->finddir(root, "ext2_stat_test", &out)));

    node = out.node;
    TEST_ASSERT_NONNULL(node);

    struct vfs_stat empty_stat = {0};
    struct vfs_stat stat_out = {0};
    TEST_ASSERT_EQ_S(node->ops->stat(node, &stat_out), ERR_OK);

    /* this should return something */
    TEST_ASSERT_NE(memcmp(&stat_out, &empty_stat, sizeof(struct vfs_stat)), 0);

    flush();
    return TEST_SUCCESS;
}

TEST_DEFINE_INTEGRATION(ext2, rename, TEST_INTENSITY(1, 1, 16),
                        .required_fs = FS_EXT2) {
    EXT2_ROOT;
    TEST_ASSERT(!ERR_IS_FATAL(
        root->ops->create(root, "ext2_rename_test", VFS_MODE_FILE)));

    struct vfs_dirent out;
    struct vfs_node *node;

    TEST_ASSERT(
        !ERR_IS_FATAL(root->ops->finddir(root, "ext2_rename_test", &out)));

    node = out.node;
    TEST_ASSERT_NONNULL(node);

    TEST_ASSERT(!ERR_IS_FATAL(node->ops->rename(root, "ext2_rename_test", root,
                                                "ext2_rename_test2")));

    enum err e = root->ops->finddir(root, "ext2_rename_test", &out);
    TEST_ASSERT_EQ_S(e, ERR_NO_ENT);

    TEST_ASSERT(
        !ERR_IS_FATAL(root->ops->finddir(root, "ext2_rename_test2", &out)));

    node = out.node;
    TEST_ASSERT_NONNULL(node);

    flush();
    return TEST_SUCCESS;
}

TEST_DEFINE_INTEGRATION(ext2, chmod, TEST_INTENSITY(1, 1, 16),
                        .required_fs = FS_EXT2) {
    EXT2_ROOT;
    TEST_ASSERT(!ERR_IS_FATAL(
        root->ops->create(root, "ext2_chmod_test", VFS_MODE_FILE)));

    struct vfs_node *node;
    struct vfs_dirent ent;

    TEST_ASSERT(
        !ERR_IS_FATAL(root->ops->finddir(root, "ext2_chmod_test", &ent)));

    node = ent.node;
    TEST_ASSERT_NONNULL(node);

    TEST_ASSERT(
        !ERR_IS_FATAL(root->ops->chmod(node, (uint16_t) VFS_MODE_O_EXEC)));
    TEST_ASSERT(node->mode & VFS_MODE_O_EXEC);

    TEST_ASSERT(
        !ERR_IS_FATAL(root->ops->finddir(root, "ext2_chmod_test", &ent)));

    node = ent.node;
    TEST_ASSERT_NONNULL(node);
    TEST_ASSERT(node->mode & VFS_MODE_O_EXEC);

    flush();
    return TEST_SUCCESS;
}

TEST_DEFINE_INTEGRATION(ext2, symlink, TEST_INTENSITY(1, 1, 16),
                        .required_fs = FS_EXT2) {
    EXT2_ROOT;
    TEST_ASSERT(
        !ERR_IS_FATAL(root->ops->symlink(root, "/tmp", "ext2_symlink_test")));

    struct vfs_dirent ent;
    struct vfs_node *node;

    TEST_ASSERT(
        !ERR_IS_FATAL(root->ops->finddir(root, "ext2_symlink_test", &ent)));

    node = ent.node;
    TEST_ASSERT_NONNULL(node);

    char *buf = valloc(5, ALLOC_ZERO);
    TEST_ASSERT_NONNULL(buf);

    TEST_ASSERT(!ERR_IS_FATAL(node->ops->readlink(node, buf, 4)));

    TEST_ASSERT_STR_EQ(buf, "/tmp");

    flush();
    return TEST_SUCCESS;
}

TEST_DEFINE_INTEGRATION(ext2, mkdir_rmdir, TEST_INTENSITY(1, 1, 16),
                        .required_fs = FS_EXT2) {
    EXT2_ROOT;
    TEST_ASSERT(
        !ERR_IS_FATAL(root->ops->mkdir(root, "ext2_dir_test", VFS_MODE_DIR)));

    struct vfs_dirent ent;
    struct vfs_node *node;

    TEST_ASSERT(!ERR_IS_FATAL(root->ops->finddir(root, "ext2_dir_test", &ent)));

    node = ent.node;
    TEST_ASSERT_NONNULL(node);

    TEST_ASSERT(!ERR_IS_FATAL(root->ops->rmdir(root, "ext2_dir_test")));

    flush();
    return TEST_SUCCESS;
}
