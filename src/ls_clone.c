#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <getopt.h>
#include <stdarg.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <dirent.h>

#define MAXLINE 2048
#define DEFAULT_PERM "----------"

static char* DEFAULT_FILES[] = {"."};
struct arg_t {
    char **files;
    int fc;             
} g_args;

static void err_sys(const char *fmt, ...)
{
    va_list ap;
    char buf[MAXLINE];

    va_start(ap, fmt);
    vsnprintf(buf, MAXLINE, fmt, ap);
    snprintf(buf+strlen(buf), MAXLINE-strlen(buf), ": %s", strerror(errno));
    strcat(buf, "\n");
    fflush(stdout); 
    fputs(buf, stderr);
    fflush(NULL);
    va_end(ap);
}

static void arg_init(struct arg_t *arg)
{
    arg->files = DEFAULT_FILES;
    arg->fc = 1;
}

static int cmp(const void *pa, const void *pb)
{
    char *s1 = *(char**)pa;
    char *s2 = *(char**)pb;

    return strcasecmp(s1, s2);
}

static void sort(char **files, int n)
{
    qsort(files, n, sizeof(char*), cmp);
}

static int isdir(const char *path)
{
    struct stat buf;
    int ret = stat(path, &buf);

    return ret == 0 && S_ISDIR(buf.st_mode);
}

static void classify(char **dirs, int *dc, char **files, int *fc)
{
    int i, di, fi;

    for (i = di = fi = 0; i < g_args.fc; i++) {
        if (isdir(g_args.files[i])) {
            dirs[di++] = g_args.files[i];
        } else {
            files[fi++] = g_args.files[i];
        }
    }
    *dc = di;
    *fc = fi;
}

static char* fmt_mode(mode_t mode)
{
    static char ms[11];
    enum {
        TYPE,
        UR, UW, UX,
        GR, GW, GX,
        OR, OW, OX
    };

    strcpy(ms, DEFAULT_PERM);
    if (S_ISDIR(mode))  ms[TYPE] = 'd';
    else if (S_ISLNK(mode))  ms[TYPE] = 'l';
    else if (S_ISCHR(mode)) ms[TYPE] = 'c';
    else if (S_ISBLK(mode)) ms[TYPE] = 'b';
    else if (S_ISFIFO(mode)) ms[TYPE] = 'p';
    else if (S_ISSOCK(mode)) ms[TYPE] = 's';

    if ((S_IRUSR & mode) == S_IRUSR)
        ms[UR] = 'r';
    if ((S_IWUSR & mode) == S_IWUSR)
        ms[UW] = 'w';
    if ((S_IXUSR & mode) == S_IXUSR)
        ms[UX] = 'x';
    if ((S_IRGRP & mode) == S_IRGRP)
        ms[GR] = 'r';
    if ((S_IWGRP & mode) == S_IWGRP)
        ms[GW] = 'w';
    if ((S_IXGRP & mode) == S_IXGRP)
        ms[GX] = 'x';
    if ((S_IROTH & mode) == S_IROTH)
        ms[OR] = 'r';
    if ((S_IWOTH & mode) == S_IWOTH)
        ms[OW] = 'w';
    if ((S_IXOTH & mode) == S_IXOTH)
        ms[OX] = 'x';

    return ms;
}

static char* fmt_owner(uid_t uid)
{
    struct passwd *pwd = getpwuid(uid);
    return (pwd == NULL) ? "" : pwd->pw_name;
}

static char* fmt_group(gid_t gid)
{
    struct group *grp = getgrgid(gid);

    return (grp == NULL) ? "" : grp->gr_name;
}

static char* fmt_time(time_t *time)
{
    static char buf[64];
    struct tm *t = localtime(time);
    strftime(buf, sizeof(buf), "%F %H:%M", t);

    return buf;
}

static char* fmt_name(struct stat *st, char *name)
{
    char *buf;

    if (S_ISLNK(st->st_mode)) {
        ssize_t len;
        int offset;

        buf = malloc(4096);
        if (buf == NULL) {
            err_sys("ls: no memory");
            exit(1);
        }
        offset = snprintf(buf, 4096, "%s -> ", name);
        len = readlink(name, buf+offset, 4096-offset);
        if (len == -1) {
            err_sys("ls: can not read link %s", name);
            exit(1);
        }
        buf[offset+len] = '\0';
    } else {
        buf = strdup(name);
    }

    return buf;
}

static char* fmt_size(off_t size)
{
    static char buf[32];
    snprintf(buf, sizeof(buf), "%7ld", size);

    return buf;
}

static void print_line(struct stat *buf, char *name)
{
    char *fname;

    fprintf(stdout, "%s ", fmt_mode(buf->st_mode));
    fprintf(stdout, "%-2d ", buf->st_nlink);
    fprintf(stdout, "%-8s ", fmt_owner(buf->st_uid));
    fprintf(stdout, "%-8s ", fmt_group(buf->st_gid));
    fprintf(stdout, "%s ", fmt_size(buf->st_size));
    fprintf(stdout, "%s ", fmt_time(&buf->st_mtime));
    fname = fmt_name(buf, name);
    fprintf(stdout, "%-s", fname);
    free(fname);
    fprintf(stdout, "\n");
}

static void do_files(char **files, int fc)
{
    int i;
    struct stat buf;

    for (i = 0; i < fc; i++) {
        char *file = files[i];
        if (stat(file, &buf) != 0) {
            err_sys("ls: can not access %s", file);
        } else {
            print_line(&buf, file);
        }
    }
}

static char** listdir(char *dir, int *pn)
{
    char **files;
    int n = 32, off = 0;
    DIR *dp;
    struct dirent *dirp;

    files = malloc(sizeof(char*) * n);
    if (files == NULL) {
        err_sys("ls: no memory");
        exit(1);
    }
    dp = opendir(dir);
    if (dp == NULL) {
        err_sys("ls: can not access %s", dir);
        return NULL;
    }
    while ((dirp = readdir(dp)) != NULL) {
        if (off >= n) {
            n *= 2;
            files = realloc(files, sizeof(char*) * n);
        }
        files[off++] = strdup(dirp->d_name);
    }
    closedir(dp);
    *pn = off;
    return files;
}

static void do_dirs(char **dirs, int dc)
{
    int i;
    char *cwd;

    cwd = malloc(4096 * sizeof(char));
    if (getcwd(cwd, 4096) == NULL) {
        err_sys("ls: can not get current dir");
        exit(1);
    }

    for (i = 0; i < dc; i++) {
        char *dir = dirs[i], **files;
        int fc, j;

        files = listdir(dir, &fc);
        if (files == NULL) {
            continue;
        }
        sort(files, fc);
        if (chdir(dir) != 0) {
            err_sys("ls: can not chdir");
            goto cleanup;
        }
        if (dc > 0 && g_args.fc > 1) {
            fprintf(stdout, "\n%s:\n", dir);
        }
        do_files(files, fc);
        if (chdir(cwd) != 0) {
            err_sys("ls: can not chdir");
            exit(1);
        }
cleanup:
        for (j = 0; j < fc; j++) {
            free(files[j]);
        }
        free(files);
    }
    free(cwd);
}

int main(int argc, char **argv)
{
    char **dirs, **files;
    int fc, dc;

    dirs = malloc(sizeof(char*) * g_args.fc);
    files = malloc(sizeof(char*) * g_args.fc);
    if (dirs == NULL || files == NULL) {
        free(dirs);
        free(files);
        return 1;
    }
    arg_init(&g_args);

    classify(dirs, &dc, files, &fc);

    do_dirs(dirs, dc);

    free(dirs);
    free(files);

    return EXIT_SUCCESS;
}
