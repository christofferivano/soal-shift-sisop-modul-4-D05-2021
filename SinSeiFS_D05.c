#define FUSE_USE_VERSION 28
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>

#define MAXPATHLEN 1000
#define MAXLOGLEN 3000

static const char *dirpath = "/home/aulia/Documents";
char *logpath = "/home/aulia/SinSeiFS.log";

void writeW(char* str){
    FILE* logFile = fopen(logpath, "a+");
    time_t t;
    time(&t);
    struct tm *timeinfo = localtime(&t);
    fprintf(logFile, "WARNING::%02d%02d%04d-%02d:%02d:%02d:%s\n", timeinfo->tm_mday, timeinfo->tm_mon+1, timeinfo->tm_year+1900, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, str);
    fclose(logFile);
}

void writeI(char* str){
    FILE* logFile = fopen(logpath, "a+");
    time_t t;
    time(&t);
    struct tm *timeinfo = localtime(&t);
    fprintf(logFile, "INFO::%02d%02d%04d-%02d:%02d:%02d:%s\n", timeinfo->tm_mday, timeinfo->tm_mon+1, timeinfo->tm_year+1900, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, str);
    fclose(logFile);
}

void encode1 (char* path, int len) {
    if (strcmp(path, ".") == 0 || strcmp(path, "..") == 0)
        return;

    for (int i=len-1; i>=0; i--){
        if (path[i] == '/') {
            break;
        } else if (path[i] == '.') {
            len = i;
            break;
        }
    }

    for (int i=0; i<len; i++) {
        int ascii = (int) path[i];
        if (ascii>=65 && ascii<=90){
            ascii = 65 + 25 - (ascii - 65);
        } else if (ascii>=97 && ascii<=122){
            ascii = 97 + 25 - (ascii - 97);
        }
        path[i] = (char) ascii;
    }
}

void decode1 (char* path, int len) {
    if (strcmp(path, ".") == 0 || strcmp(path, "..") == 0){
		return;
    } 
    
    for (int i=len-1; i>=0; i--){
        if (path[i] == '/')
            break;
        if (path[i] == '.') {
            len = i;
            break;
        }
    }
    
    int start = len;
    for (int i=1; i<len; i++){
        if (path[i] == '/'){
            start = i+1;
            break;
        }
    }
    
    for (int i=start; i<len; i++) {
        int ascii = (int) path[i];
        if (ascii>=65 && ascii<=90){
            ascii = 65 + 25 - (ascii - 65);
        } else if (ascii>=97 && ascii<=122){
            ascii = 97 + 25 - (ascii - 97);
        }
        path[i] = (char) ascii;
    }
}

//Get file attributes
static int xmp_getattr (const char *path, struct stat *stbuf){
    int res;
    char fpath[MAXPATHLEN];

    printf("getattr %s%s start\n", dirpath, path);
    
    // mencari '/AtoZ_'
    char* atozptr = strstr(path, "/AtoZ_");
    if (atozptr != NULL){
        decode1(atozptr, strlen(atozptr));
        printf("getattr : decode to %s%s\n", dirpath, path);
    }

    // set fullpath
    if (strcmp(path, "/") == 0){
        path = dirpath;
        sprintf(fpath, "%s", path);
    } else {
        sprintf(fpath, "%s%s", dirpath, path);
    }

    // get attribute
    res = lstat(fpath, stbuf);
    if (res == -1){
        printf("getattr fail\n\n");
        return -errno;
    } else {
        printf("getattr finish\n\n");
    }
    
    return 0;
}

//Read directory
static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi){
    char fpath[MAXPATHLEN];
    
    printf("readdir %s%s start\n", dirpath, path);
    
    // mencari '/AtoZ_'
    char* atozptr = strstr(path, "/AtoZ_");
    if (atozptr != NULL){
        decode1(atozptr, strlen(atozptr));
        printf("readdir : decode to %s%s\n", dirpath, path);
    }
    
    // set fullpath 
    if (strcmp(path, "/") == 0){
        path = dirpath;
        sprintf(fpath, "%s", path);
    } else {
        sprintf(fpath, "%s%s", dirpath, path);
    }
    
    int res = 0;

    DIR *dp;
    struct dirent *de;
    (void) offset;
    (void) fi;
    
    dp = opendir(fpath);
    
    if (dp == NULL) {
        printf("readdir fail\n\n");
        return -errno;
    }
    
    while ((de = readdir(dp)) != NULL) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        
        // encode jika '/AtoZ_' ditemukan
        if (atozptr != NULL){
            printf("readdir : encode %s/%s to ", fpath, de->d_name);
            encode1(de->d_name, strlen(de->d_name));
            printf("%s/%s\n", fpath, de->d_name);
        }
        
        // read directory
        res = (filler(buf, de->d_name, &st, 0));
        
        if (res != 0) 
        break;
    }
    
    closedir(dp);
    printf("readdir finish\n\n");
    
    return 0;
}

//Create a directory
static int xmp_mkdir(const char *path, mode_t mode){
    int res;
    char fpath[MAXPATHLEN];
    char log[MAXLOGLEN];
    
    printf("mkdir %s%s start\n", dirpath, path);
    
    char* atozptr = strstr(path, "/AtoZ_");
    if (atozptr != NULL){
        int len = strlen(atozptr);
        for (int i = len-1; i >= 0; i--){
            if(atozptr[i] == '/'){
                len = i;
                break;
            }
        }
        decode1(atozptr, len);
        printf("mkdir: decode to %s%s\n", dirpath, path);
	}
    
    sprintf(fpath, "%s%s", dirpath, path);
    res = mkdir(fpath, mode);

    sprintf(log, "%s::%s", "MKDIR", fpath);
    writeI(log);
    
    if (res == -1){
        printf("mkdir fail\n\n");
        return -errno;
    } else {
        printf("mkdir finish\n\n");
    }	
    
    return 0;
}

//Create a file node
static int xmp_mknod(const char *path, mode_t mode, dev_t rdev){
    int res;
    char fpath[MAXPATHLEN];
    char log[MAXLOGLEN];
    
    printf("mknod %s%s start\n", dirpath, path);
    
    char* atozptr = strstr(path, "/AtoZ_");
    if (atozptr != NULL){
        int len = strlen(atozptr);
        for (int i=len-1; i >= 0; i--){
            if(atozptr[i] == '/'){
                len = i;
                break;
            }
        }
        decode1(atozptr, len);
    }
    
    sprintf(fpath, "%s%s", dirpath, path);	
    if (S_ISREG(mode)) {
        res = open(fpath, O_CREAT | O_EXCL | O_WRONLY, mode);
        if (res >= 0)
            res = close(res);
    } else if (S_ISFIFO(mode)) {
        res = mkfifo(fpath, mode);
    } else {
        res = mknod(fpath, mode, rdev);
    }
    
    sprintf(log, "%s::%s", "CREATE", fpath);
    writeI(log);
    
    if (res == -1) {
        printf("mknod fail\n\n");
        return -errno;
    } else {
        printf("mknod finish\n\n");
    }
    
    return 0;
}

//Remove a directory
static int xmp_rmdir (const char *path){
    int res;
    char fpath[MAXPATHLEN];
    char log[MAXLOGLEN];

    printf("rmdir %s%s start\n", dirpath, path);

    char* atozptr = strstr(path, "/AtoZ");
	if (atozptr != NULL){
        decode1(atozptr, strlen(atozptr));
        printf("rmdir : decode to %s%s start\n", dirpath, path);
    }
    
    sprintf(fpath, "%s%s", dirpath, path);
    res = rmdir(fpath);
    
    sprintf(log, "%s::%s", "RMDIR", fpath);
    writeW(log);
    
    if (res == -1) {
        printf("rmdir fail\n\n");
        return -errno;
    } else {
        printf("rmdir finish\n\n");
    }
    
    return 0;
}

//Remove a file
static int xmp_unlink (const char *path){
    int res;
    char fpath[MAXPATHLEN];
    char log[MAXLOGLEN];

    printf("unlink %s%s start\n", dirpath, path);

    char* atozptr = strstr(path, "/AtoZ_");
    if (atozptr != NULL){
        decode1(atozptr, strlen(atozptr));
        printf("unlink : decode to %s%s\n", dirpath, path);
    }

    sprintf(fpath, "%s%s", dirpath, path);    
    res = unlink(fpath);

    sprintf(log, "%s::%s", "REMOVE", fpath);
    writeW(log);

    if(res == -1){
        printf("unlink fail\n\n");
        return -errno;
    } else {
        printf("unlink finish\n\n");
    }

    return 0;
}

//Rename a file
static int xmp_rename (const char *srcpath, const char *destpath){
    int res;
    char fsrcpath[MAXPATHLEN];
    char fdestpath[MAXPATHLEN];
    char log[MAXLOGLEN];

    printf("rename %s%s to %s%s start\n", dirpath, srcpath, dirpath, destpath);

    // mencari '/AtoZ_' pada scrpath dan decode jika ada
    char* atozptrsrc = strstr(srcpath, "/AtoZ_");
    if (atozptrsrc != NULL){
        decode1(atozptrsrc, strlen(atozptrsrc));
        printf("rename : decode srcpath to %s%s\n", dirpath, srcpath);
    }

    // mencari '/AtoZ_' pada destpath dan bila ada, decode path sebelum nama file/dir yg diinginkan
    char* atozptrdest = strstr(destpath, "/AtoZ_");
    if (atozptrdest != NULL){
        int len = strlen(atozptrdest);
        for (int i = len-1; i >= 0; i--){
            if(atozptrdest[i] == '/'){
                len = i;
                break;
            }
        }
        decode1(atozptrdest, len);
        printf("rename : decode destpath to %s%s\n", dirpath, destpath);
    }

    // set fullpath dan rename
    sprintf(fsrcpath, "%s%s", dirpath, srcpath);
    sprintf(fdestpath, "%s%s", dirpath, destpath);
    res = rename(fsrcpath, fdestpath);

    // tulis log
    sprintf(log, "%s::%s::%s", "RENAME", fsrcpath, fdestpath);
    writeI(log);

    if (res == -1){
        printf("rename fail\n\n");
        return -errno;
    } else {
        printf("rename finish\n\n");
    }

    return 0;
}

//Change the size of a file
static int xmp_truncate (const char *path, off_t size) {
    int res;
    char fpath[MAXPATHLEN];

    printf("truncate %s%s start\n", dirpath, path);
    
    char* atozptr = strstr(path, "/AtoZ_");
    if (atozptr != NULL){
        decode1(atozptr, strlen(atozptr));
        printf("truncate decode to %s%s\n", dirpath, path);
    }
    
    sprintf(fpath, "%s%s", dirpath, path);
    res = truncate(fpath, size);
    
    if (res == -1){
        printf("truncate fail\n\n");
        return -errno;
    } else {
        printf("truncate finish\n\n");
    }
    
    return 0;
}

//File open operation
static int xmp_open(const char *path, struct fuse_file_info *fi){
    int res;
    char fpath[MAXPATHLEN];

    printf("open %s%s start\n", dirpath, path);

    char* atozptr = strstr(path, "/AtoZ_");
    
    if (atozptr != NULL){
        decode1(atozptr, strlen(atozptr));
        printf("open : decode to %s%s\n", dirpath, path);
    }
    
    sprintf(fpath, "%s%s", dirpath, path);
    res = open(fpath, fi->flags);
    
    if (res == -1) {
        printf("open fail\n\n");
        return -errno;
    } else {
        close(res);
        printf("open finish\n\n");
    }
    
    return 0;
}

//Read data from an open file
static int xmp_read (const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
    char fpath[MAXPATHLEN];

    printf("read %s%s start\n", dirpath, path);

    char* atozptr = strstr(path, "/AtoZ_");
    if (atozptr != NULL){
        decode1(atozptr, strlen(atozptr));
        printf("read : decode to %s%s\n", dirpath, path);
    }

    sprintf(fpath, "%s%s", dirpath, path);
    
    int res = 0;
    int fd = 0;
    (void) fi;

    fd = open(fpath, O_RDONLY);
    if (fd == -1) {
        printf("read fail\n\n");
        return -errno;
    }

    res = pread(fd, buf, size, offset);
    if (res == -1) {
        close(fd);
        printf("read fail\n\n");
        return -errno;
    } else {
        close(fd);
        printf("read finish\n\n");
    }

    return 0;
}

//Write data to an open file
static int xmp_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    char fpath[MAXPATHLEN];
    char log[MAXLOGLEN];

    printf("write %s%s start\n", dirpath, path);

    char* atozptr = strstr(path, "/AtoZ_");
    if (atozptr != NULL){
        decode1(atozptr, strlen(atozptr));
        printf("write : decode to %s%s\n", dirpath, path);
    }
    
    sprintf(fpath, "%s%s", dirpath, path);
    
    int fd;
    int res;
    (void) fi;
    
    fd = open(fpath, O_WRONLY);
    if (fd == -1){
        printf("write fail\n\n");
        return -errno;
    }
    
    res = pwrite(fd, buf, size, offset);

    sprintf(log, "%s::%s", "WRITE", fpath);
    writeI(log);
    
    if (res == -1){
        close(fd);
        printf("write fail\n\n");
        return -errno;
    } else {
        close(fd);
        printf("write finish\n\n");
    }
    
    return 0;
}

static struct fuse_operations xmp_oper = {
    .getattr = xmp_getattr,
    .readdir = xmp_readdir,
    .mkdir = xmp_mkdir, 
    .mknod = xmp_mknod,
    .rmdir = xmp_rmdir,
    .unlink = xmp_unlink,
    .rename = xmp_rename,
    .truncate = xmp_truncate,
    .open = xmp_open,
    .read = xmp_read,
    .write = xmp_write,
};

int main(int argc, char *argv[]) {
    umask(0);
    return fuse_main(argc, argv, &xmp_oper, NULL);
}
