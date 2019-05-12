# _<div align="center">\~ FINAL PROJECT SISOP E07 \~</div>_
#### _<div align="center">Because ten billion years' time is so fragile, so ephemeral... it arouses such a bittersweet, almost heartbreaking fondness.</div>_
> Buat sebuah program FUSE yang mengumpulkan berbagai MP3 di satu folder.
> MP3 tersebar di "/home/$USER".
> MP3 tersebut akan muncul di _mountpoint_ tanpa ada subfolder lainnya.
> Buat juga MP3 player yang mempunyai fitur _play_, _pause_, _next_, _prev_, _find_, dan _list_.
> MP3 player tersebut diarahkan ke _mountpoint_
>
> _--Soal FP Sisop 2019_
## _\~ Bagian 1, MP3 Player_
Copas dulu dari [sini](https://hzqtc.github.io/2012/05/play-mp3-with-libmpg123-and-libao.html).
_Thread_ bertugas untuk menjalankan dan menghentikan musik, tergantung dari _input_.
_Linked list_ digunakan untuk menyimpan nama-nama lagu yang ada di folder.

Compile dengan
```bash
gcc player.c -o player -pthread
```
## _\~ Bagian 2, FUSE_
**1) Compile**
```bash
gcc fuse.c -o fuse `pkg-config fuse --cflags --libs`
```

**2) struct fuse_operations**

Hampir semua fungsi diimplementasikan. Hal ini untuk mencegah adanya _silent error_ yang sulit di-_debug_.
Listnya sebagai berikut :
```C
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
```
Semua fungsi tersebut diambil dari `fuse.h`. 


**3) Ringkasan cara kerja :**

1. Cari semua mp3 yang ada di `/home/$USER`.

   Cara mencarinya menggunakan [nftw()](https://pubs.opengroup.org/onlinepubs/9699919799/functions/nftw.html). Hasil dari
   `nftw()` akan disimpan di `/home/$USER/db.txt`, berisi _absolute path_ dari semua mp3 yang ada di `/home/$USER`
2. Proses db.txt

   Menggunakan [getline()](https://pubs.opengroup.org/onlinepubs/9699919799/functions/getdelim.html) untuk membaca setiap baris.
   Untuk setiap baris, ekstrak nama dari mp3nya. Kemudian `push_back(key, value)` ke _queue_ dengan _key_ berisi nama mp3 dan
   _value_ berisi _absolute path_ dari mp3.
3. Tampilkan mp3 di _mountpoint_

   Dengan bantuan fungsi `filler()` dari FUSE sendiri, tampilkan semua _key_ yang ada di _queue_.
4. Translasi _request_

   Untuk setiap _request_ yang menuju fail mp3, cari dulu _absolute path_-nya di queue. _Absolute path_ inilah yang akan
   diberikan ke fungsi yang bersangkutan. Jika yang dituju bukan fail, _prepend_ saja dengan root. Root di sini adalah
   _path_ yang dipilih untuk ditampilkan di _mountpoint_, untuk kasus ini `/home/$USER`
   
# _<div align="center">~ FIN ~</div>_
