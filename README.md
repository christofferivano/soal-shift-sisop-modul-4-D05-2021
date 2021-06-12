# soal-shift-sisop-modul-4-D05-2021
## 1. Encode Direktori AtoZ_
Pada soal ini kita diminta untuk meng-`encode`-kan semua nama file (kecuali ekstensi) dan direktori yang berada di dalam direktori yang nama nya diawali dengan `AtoZ_` menggunakan `atbash chipper (mirror)` dan berlaku secara rekursif. Selain itu, apabila direktori dengan nama yang diawali dengan `AtoZ_` di-`rename` sehingga namanya tidak lagi diawali dengan `AtoZ_`, maka nama file dan direktori yang berada di dalam direktori tersebut akan di-`decode` kembali. Dan semua pembuatan direkrori ter-encode (yang berawlan `AtoZ_`), baik mkdir maupun rename, akan dicatat dalam sebuah log.

Untuk menyelesaikan soal ini, kita dapat membuat fungsi `encode1` untuk meng-encode-kan nama file dan direktori. Fungsi `encode1` ini akan dipanggil ketika readdir dipanggil, sehingga nama file dan folder yang ditampilkan akan berubah menjadi ter-encode. 
```
void encode1 (char* path, int len) {
    if (strcmp(path, ".") == 0 || strcmp(path, "..") == 0)
        return;

    // mencari extensi file    
    for (int i=len-1; i>=0; i--){
        if (path[i] == '/') {
            break;
        } else if (path[i] == '.') {
            len = i;
            break;
        }
    }

    // mengencodekan path dengan mirror chipper
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
```

Kemudian, untuk men-decode-kan nama file dan direktori, kita dapat melakukan hal yang sama seperti encode. Fungsi yang dibuat adalah `decode1`. Yang membedakan antara `encode1` dan `decode1` adalah input path nya. Path yang diinputkan dalam `encode1` adalah nama file atau direktori, sedangkan path yang diinputkan dalam `decode1` adalah pointer yang menunjuk pada `/AtoZ_` dalam sebuah path. Sehingga perlu ditambahkan code untuk mencari nama file/direktori setelah `/AtoZ`.
```
void decode1 (char* path, int len) {
    if (strcmp(path, ".") == 0 || strcmp(path, "..") == 0){
        return;
    } 
    
    // mencari ektensi file
    for (int i=len-1; i>=0; i--){
        if (path[i] == '/')
            break;
        if (path[i] == '.') {
            len = i;
            break;
        }
    }
    
    // mencari '/' setelah '/AtoZ_'
    int start = len;
    for (int i=1; i<len; i++){
        if (path[i] == '/'){
            start = i+1;
            break;
        }
    }
    
    // mendecodekan path dengan mirror chipper
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
```

Adapun struct `fuse_operations` yang diimplementasikan untuk pengerjaan soal ini adalah sebagai berikut.
```
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
```

Untuk get attribute, kita dapat mengimplementasikan fungsi `xmp_getattr` dan memanggil fungsi `decode1` jika ditemukan `/AtoZ` dalam path nya.
```
static int xmp_getattr (const char *path, struct stat *stbuf){
    int res;
    char fpath[MAXPATHLEN];

    printf("getattr %s%s start\n", dirpath, path);
    
    // mencari '/AtoZ_' dan decode jika ada
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
```

Untuk read directory, kita dapat mengimplementasikan fungsi `xmp_readdir` dan memanggil fungsi `encode1` untuk meng-encode nama file/direktori di dalamnya jika ditemukan `/AtoZ_` pada path direktori tersebut.
```
static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi){
    char fpath[MAXPATHLEN];
    
    printf("readdir %s%s start\n", dirpath, path);
    
    // mencari '/AtoZ_' dan decode jika ada
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
```

Untuk make directory, kita dapat mengimplementasikan fungsi `xmp_mkdir` dan memanggil fungsi `decode1` untuk men-decode path sebelum nama direktori yang dibuat serta menulis log jika `AtoZ` ditemukan dalam path nya. 
```
static int xmp_mkdir(const char *path, mode_t mode){
    int res;
    char fpath[MAXPATHLEN];
    char log[MAXLOGLEN];
    
    printf("mkdir %s%s start\n", dirpath, path);
    
    // mencari 'AtoZ_'. jika ada, decode path selain nama dir yang akan dibuat 
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
    
    // set fullpath dan make directory
    sprintf(fpath, "%s%s", dirpath, path);
    res = mkdir(fpath, mode);

    // tulis log
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
```

Untuk rename, kita dapat mengimplementasikan fungsi `xmp_rename` dan memanggil fungsi `decode1` untuk men-decode path serta menulis log jika ditemukan `/AtoZ_` di `srcpath` atau `destpath`.
```
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
```

Hasil yang diperoleh setelah membuat direktori `AtoZ_folder1` beserta sub-sub direktorinya dan me-rename nama direktori `AtoZ_folder1` menjadi `folder1` adalah sebagai berikut. Sedangkan hasil log akan ditampilkan di pengerjaan nomor 4.
![AtoZ_folder1](https://user-images.githubusercontent.com/76677130/121776815-22502c80-cbb9-11eb-9cac-2502314fd0e0.png)
![folder1](https://user-images.githubusercontent.com/76677130/121776819-24b28680-cbb9-11eb-8a34-a239bc6ada58.png)

Adapun kendala yang dialami selama pengerjaan adalah sebagai berikut.
1. Untuk pembuatan direktori muncul error, namun direktori berhasil terbuat.
2. Write file memunculkan pesan error.

## 2.

## 3.

## 4. Log
Pada soal ini, kita diminta untuk membuat file log bernama `SinSeiFS.log` yang diletakkan pada direktori home pengguna. Isi log terdiri dari dua level, yaitu `WARNING` untuk system call rmdir dan unlink, dan `INFO` untuk system call lainnya. Adapun format dari log tersebut adalah sebagai berikut.
```
[LEVEL]::[DD]:[MM]:[YYYY]-[HH]:[MM]:[SS]:[CMD]::[DESC::DESC]
```

Untuk menyelesaikan soal ini, kita dapat membuat fungsi `writeW` untuk menuliskan log dengan level `WARNING`. Fungsi ini nantinya akan dipanggil dari dalam fungsi `xmp_rmdir` dan `xmp_unlink`, dengan parameter berupa gabungan `[CMD]` dengan `[DESC]`. `[CMD]` berisi `WARNING` dan `[DESC]` berisi `fpath`.
```
void writeW(char* str){
    FILE* logFile = fopen(logpath, "a+");
    time_t t;
    time(&t);
    struct tm *timeinfo = localtime(&t);
    fprintf(logFile, "WARNING::%02d%02d%04d-%02d:%02d:%02d:%s\n",
        timeinfo->tm_mday, timeinfo->tm_mon+1, timeinfo->tm_year+1900,
        timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, str);
    fclose(logFile);
}
```

Selain fungsi `writeW`, kita juga harus membuat fungsi `writeI` untuk menuliskan log dengan level `INFO`. Fungsi ini nantinya akan dipanggil dari dalam fungsi selain `xmp_rmdir` dan `xmp_unlink`, dengan parameter berupa gabungan `[CMD]` dengan `[DESC]` dan `[DESC]` lainnya jika ada. `[CMD]` berisi `INFO` dan `[DESC]` berisi `fpath`.
```
void writeI(char* str){
    FILE* logFile = fopen(logpath, "a+");
    time_t t;
    time(&t);
    struct tm *timeinfo = localtime(&t);
    fprintf(logFile, "INFO::%02d%02d%04d-%02d:%02d:%02d:%s\n", 
        timeinfo->tm_mday, timeinfo->tm_mon+1, timeinfo->tm_year+1900, 
        timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, str);
    fclose(logFile);
}
```

Hasil file log yang diperoleh adalah sebagai berikut.
![Screenshot (955)](https://user-images.githubusercontent.com/76677130/121776887-74914d80-cbb9-11eb-8392-478448a2ec74.png)

Dalam pengerjaan soal ini, tidak ada kendala yang dialami.
