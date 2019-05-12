// ***************************
// HOW TO COMPILE
// gcc -D_GNU_SOURCE -D_XOPEN_SOURCE=500 `pkg-config fuse --cflags --libs`
//     fuse.c -o fuse
// **************************

#define FUSE_USE_VERSION 26
#define _GNU_SOURCE
#define _XOPEN_SOURCE 500
#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/xattr.h>
#include <sys/time.h>
#include <ftw.h>
#define MAX_PATH 260

const char root[] = "/home/arino";

struct node
{
    char *key, *value;
    struct node *next, *prev;
};

struct queue
{
    struct node *head, *tail;
    size_t size;
};

struct my_data
{
    struct queue *qp;
};

struct queue* init_queue()
{
    struct queue *qp = calloc(1, sizeof *qp);
    qp->head = qp->tail = NULL;
    qp->size = 0;
    return qp;
}

void push_back(struct queue *qp, const char key[], const char value[])
{
    struct node *box = calloc(1, sizeof *box);
    box->key = calloc(1, MAX_PATH * sizeof box->key);
    box->value = calloc(1, MAX_PATH * sizeof box->value);
    strcpy(box->key, key);
    strcpy(box->value, value);
    box->next = box->prev = NULL;

    if (qp->size == 0)
    {
        qp->head = qp->tail = box;
        qp->size++;
    }
    else
    {
        qp->tail->next = box;
        box->prev = qp->tail;
        qp->tail = box;
        qp->size++;
    }
}

void find_value(struct queue *qp, char fpath[], const char key[])
{
    struct node *it = qp->head;
    while (it != NULL)
    {
        if (!strcmp(it->key, key))
            break;
        it = it->next;
    }
    strcpy(fpath, it->value);
}

void delete_value(struct queue *qp, const char key[])
{
    struct node *it = qp->head;
    while (it != NULL)
    {
        if (!strcmp(it->key, key))
            break;
        it = it->next;
    }
    it->prev->next = it->next;
    it->next->prev = it->prev;
    free(it->key);
    free(it->value);
    free(it);
}

int fn(const char *path, const struct stat *st, int flags, struct FTW *ftwbuf)
{
    (void)st;
    (void)ftwbuf;
    if (flags == FTW_F)
    {
        char *ext = strrchr(path, '.');
        if ((ext != NULL) && (!strcmp(ext, ".mp3")))
        {
            FILE *db = fopen("/home/arino/db.txt", "a+");
            fprintf(db, "%s\n", path);
            fclose(db);
        }
    }
    return 0;
}

void proc_list(struct queue *qp)
{
    FILE *db = fopen("/home/arino/db.txt", "r+");
    char *line = NULL, *tmp;
    char key[MAX_PATH] = {'\0'}, value[MAX_PATH] = {'\0'};
    ssize_t n;
    size_t len = 0;

    while ((n = getline(&line, &len, db)) != -1)
    {
        strcpy(value, line);
        tmp = strrchr(value, '\n');
        if (tmp != NULL)
            *tmp = '\0';
        tmp = strrchr(value, '/');
        strcpy(key, tmp + 1);
        push_back(qp, key, value);
        memset(key, '\0', sizeof key);
        memset(value, '\0', sizeof value);
    }
    fclose(db);
}

static int my_getattr(const char *path, struct stat *st)
{
    char fpath[MAX_PATH] = {'\0'}, *ext = strrchr(path, '.');
    if ((ext != NULL) && !strcmp(ext, ".mp3"))
        find_value(((struct my_data*)fuse_get_context()->private_data)->qp,
                   fpath, path + 1);
    else
        sprintf(fpath, "%s%s", root, path);

    int res = lstat(fpath, st);
    if (res == -1)
        return -errno;
    return 0;
}

static int my_readlink(const char *path, char *buf, size_t size)
{
    char fpath[MAX_PATH] = {'\0'}, *ext = strrchr(path, '.');
    if ((ext != NULL) && !strcmp(ext, ".mp3"))
        find_value(((struct my_data*)fuse_get_context()->private_data)->qp,
                   fpath, path + 1);
    else
        sprintf(fpath, "%s%s", root, path);

    int res = readlink(fpath, buf, size - 1);
    if (res >= 0)
        buf[res] = '\0';
    return 0;
}

static int my_mknod(const char *path, mode_t mode, dev_t dev)
{
    int res;
    char fpath[MAX_PATH] = {'\0'}, *ext = strrchr(path, '.');
    sprintf(fpath, "%s%s", root, path);
    if ((ext != NULL) && !strcmp(ext, ".mp3"))
        push_back(((struct my_data*)fuse_get_context()->private_data)->qp,
                  path + 1, fpath);

    if (S_ISREG(mode))
    {
        res = open(fpath, O_CREAT | O_EXCL | O_RDONLY, mode);
        if (res >= 0)
            close(res);
    }
    else if (S_ISFIFO(mode))
        res = mkfifo(fpath, mode);
    else
        res = mknod(fpath, mode, dev);

    if (res == -1)
        return -errno;
    return 0;
}

static int my_mkdir(const char *path, mode_t mode)
{
    char fpath[MAX_PATH] = {'\0'};
    sprintf(fpath, "%s%s", root, path);

    int res = mkdir(fpath, mode);
    if (res == -1)
        return -errno;
    return 0;
}

static int my_unlink(const char *path)
{
    char fpath[MAX_PATH] = {'\0'}, *ext = strrchr(path, '.');
    sprintf(fpath, "%s%s", root, path);
    if ((ext != NULL) && (!strcmp(ext, ".mp3")))
        delete_value(((struct my_data*)fuse_get_context()->private_data)->qp,
                     path + 1);

    int res = unlink(fpath);
    if (res == -1)
        return -errno;
    return 0;
}

static int my_rmdir(const char *path)
{
    char fpath[MAX_PATH] = {'\0'};
    sprintf(fpath, "%s%s", root, path);

    int res = rmdir(fpath);
    if (res == -1)
        return -errno;
    return 0;
}

static int my_symlink(const char *from, const char *to)
{
    char fto[MAX_PATH] = {'\0'};
    sprintf(fto, "%s%s", root, to);

    int res = symlink(from, fto);
    if (res == -1)
        return -errno;
    return 0;
}

static int my_rename(const char *from, const char *to)
{
    char ffrom[MAX_PATH] = {'\0'};
    char fto[MAX_PATH] = {'\0'};
    sprintf(ffrom, "%s%s", root, from);
    sprintf(fto, "%s%s", root, to);

    int res = rename(ffrom, fto);
    if (res == -1)
        return -errno;
    return 0;
}

static int my_link(const char *from, const char *to)
{
    char ffrom[MAX_PATH] = {'\0'};
    char fto[MAX_PATH] = {'\0'};
    sprintf(ffrom, "%s%s", root, from);
    sprintf(fto, "%s%s", root, to);

    int res = link(ffrom, fto);
    if (res == -1)
        return -errno;
    return 0;
}

static int my_chmod(const char *path, mode_t mode)
{
    char fpath[MAX_PATH] = {'\0'}, *ext = strrchr(path, '.');
    if ((ext != NULL) && !strcmp(ext, ".mp3"))
        find_value(((struct my_data*)fuse_get_context()->private_data)->qp,
                   fpath, path + 1);
    else
        sprintf(fpath, "%s%s", root, path);

    int res = chmod(fpath, mode);
    if (res == -1)
        return -errno;
    return 0;
}

static int my_chown(const char *path, uid_t uid, gid_t gid)
{
    char fpath[MAX_PATH] = {'\0'}, *ext = strrchr(path, '.');
    if ((ext != NULL) && !strcmp(ext, ".mp3"))
        find_value(((struct my_data*)fuse_get_context()->private_data)->qp,
                   fpath, path + 1);
    else
        sprintf(fpath, "%s%s", root, path);

    int res = chown(fpath, uid, gid);
    if (res == -1)
        return -errno;
    return 0;
}

static int my_truncate(const char *path, off_t len)
{
    char fpath[MAX_PATH] = {'\0'}, *ext = strrchr(path, '.');
    if ((ext != NULL) && !strcmp(ext, ".mp3"))
        find_value(((struct my_data*)fuse_get_context()->private_data)->qp,
                   fpath, path + 1);
    else
        sprintf(fpath, "%s%s", root, path);

    int res = truncate(fpath, len);
    if (res == -1)
        return -errno;
    return 0;
}

static int my_open(const char *path, struct fuse_file_info *fi)
{
    char fpath[MAX_PATH] = {'\0'}, *ext = strrchr(path, '.');
    if ((ext != NULL) && !strcmp(ext, ".mp3"))
        find_value(((struct my_data*)fuse_get_context()->private_data)->qp,
                   fpath, path + 1);
    else
        sprintf(fpath, "%s%s", root, path);

    int res = open(fpath, fi->flags);
    if (res == -1)
        return -errno;
    fi->fh = res;
    return 0;
}

static int my_read(const char *path,
                   char *buf,
                   size_t size,
                   off_t offset,
                   struct fuse_file_info *fi)
{
    (void)path;

    int res = pread(fi->fh, buf, size, offset);
    if (res == -1)
        res = -errno;
    return res;
}

static int my_write(const char *path,
                    const char *buf,
                    size_t size,
                    off_t offset,
                    struct fuse_file_info *fi)
{
    (void)path;

    int res = pwrite(fi->fh, buf, size, offset);
    if (res == -1)
        res = -errno;
    return res;
}

static int my_statfs(const char *path, struct statvfs *stv)
{
    char fpath[MAX_PATH] = {'\0'}, *ext = strrchr(path, '.');
    if ((ext != NULL) && !strcmp(ext, ".mp3"))
        find_value(((struct my_data*)fuse_get_context()->private_data)->qp,
                   fpath, path + 1);
    else
        sprintf(fpath, "%s%s", root, path);

    int res = statvfs(fpath, stv);
    if (res == -1)
        return -errno;
    return 0;
}

static int my_flush(const char *path, struct fuse_file_info *fi)
{
    (void)path;
    int res = close(dup(fi->fh));
    if (res == -1)
        return -errno;
    return 0;
}

static int my_release(const char *path, struct fuse_file_info *fi)
{
    (void)path;
    int res = close(fi->fh);
    if (res == -1)
        return -errno;
    return 0;
}

static int my_fsync(const char *path, int datasync, struct fuse_file_info *fi)
{
    int res;
    (void)path;
    if (datasync)
        res = fdatasync(fi->fh);
    else
        res = fsync(fi->fh);

    if (res == -1)
        return -errno;
    return 0;
}

static int my_setxattr(const char *path,
                       const char *name,
                       const char *value,
                       size_t size,
                       int flags)
{
    char fpath[MAX_PATH] = {'\0'}, *ext = strrchr(path, '.');
    if ((ext != NULL) && !strcmp(ext, ".mp3"))
        find_value(((struct my_data*)fuse_get_context()->private_data)->qp,
                   fpath, path + 1);
    else
        sprintf(fpath, "%s%s", root, path);

    int res = lsetxattr(fpath, name, value, size, flags);
    if (res == -1)
        return -errno;
    return 0;
}

static int my_getxattr(const char *path,
                       const char *name,
                       char *value,
                       size_t size)
{
    char fpath[MAX_PATH] = {'\0'}, *ext = strrchr(path, '.');
    if ((ext != NULL) && !strcmp(ext, ".mp3"))
        find_value(((struct my_data*)fuse_get_context()->private_data)->qp,
                   fpath, path + 1);
    else
        sprintf(fpath, "%s%s", root, path);

    int res = lgetxattr(fpath, name, value, size);
    if (res == -1)
        return -errno;
    return 0;
}

static int my_listxattr(const char *path, char *list, size_t size)
{
    char fpath[MAX_PATH] = {'\0'}, *ext = strrchr(path, '.');
    if ((ext != NULL) && !strcmp(ext, ".mp3"))
        find_value(((struct my_data*)fuse_get_context()->private_data)->qp,
                   fpath, path + 1);
    else
        sprintf(fpath, "%s%s", root, path);

    int res = llistxattr(fpath, list, size);
    if (res == -1)
        return -errno;
    return 0;
}

static int my_removexattr(const char *path, const char *name)
{
    char fpath[MAX_PATH] = {'\0'}, *ext = strrchr(path, '.');
    if ((ext != NULL) && !strcmp(ext, ".mp3"))
        find_value(((struct my_data*)fuse_get_context()->private_data)->qp,
                   fpath, path + 1);
    else
        sprintf(fpath, "%s%s", root, path);

    int res = lremovexattr(fpath, name);
    if (res == -1)
        return -errno;
    return 0;
}

static int my_opendir(const char *path, struct fuse_file_info *fi)
{
    char fpath[MAX_PATH] = {'\0'};
    sprintf(fpath, "%s%s", root, path);

    DIR *dp = opendir(fpath);
    if (dp == NULL)
        return -errno;

    fi->fh = (uintptr_t)dp;
    return 0;
}

static int my_readdir(const char *path,
                      void *buf,
                      fuse_fill_dir_t filler,
                      off_t offset,
                      struct fuse_file_info *fi)
{
    (void)path;
    (void)offset;
    (void)fi;
    struct queue *qp = ((struct my_data*)fuse_get_context()->private_data)->qp;
    struct node *it = qp->head;
    while (it != NULL)
    {
        if (filler(buf, it->key, NULL, 0))
            break;
        it = it->next;
    }
    return 0;
}

static int my_releasedir(const char *path, struct fuse_file_info *fi)
{
    (void)path;
    closedir((DIR*)(uintptr_t)fi->fh);
    return 0;
}

static void* my_init(struct fuse_conn_info *conn)
{
    (void)conn;
    return (struct my_data*)fuse_get_context()->private_data;
}

static void my_destroy(void *private_data)
{
    (void)private_data;
    remove("/home/arino/db.txt");
}

static int my_access(const char *path, int mask)
{
    char fpath[MAX_PATH] = {'\0'}, *ext = strrchr(path, '.');
    if ((ext != NULL) && !strcmp(ext, ".mp3"))
        find_value(((struct my_data*)fuse_get_context()->private_data)->qp,
                   fpath, path + 1);
    else
        sprintf(fpath, "%s%s", root, path);

    int res = access(fpath, mask);
    if (res == -1)
        return -errno;
    return 0;
}

static int my_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
    char fpath[MAX_PATH] = {'\0'}, *ext = strrchr(path, '.');
    if ((ext != NULL) && !strcmp(ext, ".mp3"))
        find_value(((struct my_data*)fuse_get_context()->private_data)->qp,
                   fpath, path + 1);
    else
        sprintf(fpath, "%s%s", root, path);

    int res = open(fpath, fi->flags, mode);
    if (res == -1)
        return -errno;
    fi->fh = res;
    return 0;
}

static int my_ftruncate(const char *path,
                        off_t len,
                        struct fuse_file_info *fi)
{
    (void)path;
    int res = ftruncate(fi->fh, len);
    if (res == -1)
        return -errno;
    return 0;
}

static int my_fgetattr(const char *path,
                       struct stat *st,
                       struct fuse_file_info *fi)
{
    if (!strcmp(path, "/"))
        return my_getattr(path, st);

    int res = fstat(fi->fh, st);
    if (res == -1)
        return -errno;
    return 0;
}

static int my_utimens(const char *path, const struct timespec tv[2])
{
    char fpath[MAX_PATH] = {'\0'}, *ext = strrchr(path, '.');
    if ((ext != NULL) && !strcmp(ext, ".mp3"))
        find_value(((struct my_data*)fuse_get_context()->private_data)->qp,
                   fpath, path + 1);
    else
        sprintf(fpath, "%s%s", root, path);

    int res = utimensat(0, fpath, tv, AT_SYMLINK_NOFOLLOW);
    if (res == -1)
        return -errno;
    return 0;
}

struct fuse_operations op =
{
    .getattr = my_getattr,
    .readlink = my_readlink,
    .getdir = NULL, //DEPRECATED
    .mknod = my_mknod,
    .mkdir = my_mkdir,
    .unlink = my_unlink,
    .rmdir = my_rmdir,
    .symlink = my_symlink,
    .rename = my_rename,
    .link = my_link,
    .chmod = my_chmod,
    .chown = my_chown,
    .truncate = my_truncate,
    .utime = NULL, //DEPRECATED
    .open = my_open,
    .read = my_read,
    .write = my_write,
    .statfs = my_statfs,
    .flush = my_flush,
    .release = my_release,
    .fsync = my_fsync,
    .setxattr = my_setxattr,
    .getxattr = my_getxattr,
    .listxattr = my_listxattr,
    .removexattr = my_removexattr,
    .opendir = my_opendir,
    .readdir = my_readdir,
    .releasedir = my_releasedir,
    .fsyncdir = NULL, //NO CLEAR USAGE
    .init = my_init, //For generating MP3s list(?)
    .destroy = my_destroy, //For deleting MP3s list(?)
    .access = my_access,
    .create = my_create,
    .ftruncate = my_ftruncate,
    .fgetattr = my_fgetattr,
    .lock = NULL, //NOT USED
    .utimens = my_utimens,
    .bmap = NULL, //NOT USED
    .ioctl = NULL, //NOT USED
    .poll = NULL, //NOT USED
    .write_buf = NULL, //Could be useful(?)
    .read_buf = NULL, //Could be useful(?)
    .flock = NULL, //NOT USED
    .fallocate = NULL //NOT USED
};

int main(int argc, char *argv[])
{
    umask(0);
    remove("/home/arino/db.txt");
    struct my_data *md = calloc(1, sizeof *md);
    struct queue *qp = init_queue();
    nftw("/home/arino", fn, 20, FTW_PHYS);
    proc_list(qp);
    md->qp = qp;
    return fuse_main(argc, argv, &op, md);
}
