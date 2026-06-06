#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

void mode_string(mode_t mode, char *str) {
    if (S_ISDIR(mode))       str[0] = 'd';
    else if (S_ISLNK(mode))  str[0] = 'l';
    else if (S_ISCHR(mode))  str[0] = 'c';
    else if (S_ISBLK(mode))  str[0] = 'b';
    else if (S_ISFIFO(mode)) str[0] = 'p';
    else if (S_ISSOCK(mode)) str[0] = 's';
    else                     str[0] = '-';

    str[1] = (mode & S_IRUSR) ? 'r' : '-';
    str[2] = (mode & S_IWUSR) ? 'w' : '-';
    str[3] = (mode & S_IXUSR) ? 'x' : '-';
    str[4] = (mode & S_IRGRP) ? 'r' : '-';
    str[5] = (mode & S_IWGRP) ? 'w' : '-';
    str[6] = (mode & S_IXGRP) ? 'x' : '-';
    str[7] = (mode & S_IROTH) ? 'r' : '-';
    str[8] = (mode & S_IWOTH) ? 'w' : '-';
    str[9] = (mode & S_IXOTH) ? 'x' : '-';
    str[10] = '\0';
}

void print_long(const char *dir, const char *name) {
    char fullpath[4096];
    snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, name);

    struct stat st;
    if (lstat(fullpath, &st) < 0) {
        perror(name);
        return;
    }
    char modes[11];
    mode_string(st.st_mode, modes);

    struct passwd *pw = getpwuid(st.st_uid);
    struct group  *gr = getgrgid(st.st_gid);
    const char *user = pw ? pw->pw_name : "?";
    const char *group = gr ? gr->gr_name : "?";

    char timebuf[64];

    struct tm *tm = localtime(&st.st_mtim.tv_sec);
    strftime(timebuf, sizeof(timebuf), "%b %e %H:%M", tm);

    printf(
        "%s %lu %s %s %ld %s %s\n",
        modes,
        (unsigned long)st.st_nlink,
        user,
        group,
        (long)st.st_size,
        timebuf,
        name
    );
}

int compare_string(const void *a, const void *b) {
    const char *str_a = *(const char **)a;
    const char *str_b = *(const  char **)b;

    const unsigned char *str1 = (const unsigned char*) str_a;
    const unsigned char *str2 = (const unsigned char*) str_b;
    
    // Skip Punctuation for sorting
    char first;
    char second;
    while ((first = *str1) && (second = *str2)) {
        if (ispunct(first) || isspace(first)) {
            str1++;
            continue;
        }

        if (ispunct(second) || isspace(second)) {
            str2++;
            continue;
        }

        if (tolower(first) != tolower(second)) {
            return tolower(first) - tolower(second);
        }

        str1++;
        str2++;
    }

    while ((first = *str1) && (ispunct(first) || isspace(first))) str1++;
    while ((second = *str2) && (ispunct(second) || isspace(second))) str2++;

    int dif =  tolower(first) - tolower(second);
    return dif ? dif : strcmp(str_a, str_b);
}


int show_all = 0;
int long_format = 0;

int main (int argc, char *argv[]) {
    int opt;

    while((opt = getopt(argc, argv, "al")) != -1){
        switch (opt) {
            case 'a':
                show_all = 1;
                break;
            case 'l':
                long_format = 1;
                break;
            default:
                fprintf(stderr, "usage: %s [-al] [path]\n", argv[0]);
                return 1;
        }
    }

    if(argc - optind > 1) {
        fprintf(stderr, "usage: %s [-al] [path]\n", argv[0]);
        return 1;
    }

    const char *path = (optind < argc) ? argv[optind] : ".";

    DIR *dir = opendir(path);
    if (!dir) {
        perror("opendir");
        return 1;
    }

    struct dirent *entry;
    size_t entry_count = 0;
    while((entry = readdir(dir)) != NULL) { 
        entry_count++;
    }

    char **entries = calloc(entry_count, sizeof(*entries));
    if(!entries) {
        perror("Failed to allocate memory using calloc");
        return 1;
    }
    
    rewinddir(dir);
    size_t i = 0;
    while((entry = readdir(dir)) != NULL && i < entry_count) {
        entries[i++] = strdup(entry->d_name);
    }

    qsort(entries, entry_count, sizeof (char *), compare_string);
       
    for(i = 0; i < entry_count; i++) {
        if (!show_all && entries[i][0] == '.') continue;
        if(long_format) {
            print_long(path, entries[i]);
        }
        else {
            printf("%s\n", entries[i]);
        }
    }

    for(i = 0; i < entry_count; i++) {
        free(entries[i]);
    } 
    
    free(entries);
    closedir(dir);
    return 0;
}
