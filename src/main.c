#define _DEFAULT_SOURCE
#define __USE_MISC
#include<stdio.h>
#include<stdint.h>
#include<dirent.h>
#include<errno.h>
#include<stdlib.h>
#include<string.h>
#include<sys/stat.h>


#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_RESET   "\x1b[0m"

int any(const struct dirent * p) {return 1;}

const char * map_color(const struct dirent * d) {
    switch (d->d_type) {
        case DT_DIR : 
            return COLOR_BLUE;
    }
    return COLOR_RESET;
}

int main(int argc, char * argv[] ) {
    const char* path = (argc>1) ? argv[1] : ".";
    DIR *dir = opendir(path);
    if(dir==nullptr) {
        perror("opendir"); 
        return 1;
    }

    struct dirent ** names;
    auto num = scandir(path, &names, &any, &alphasort); 
    printf("%d\n" , num);
    size_t maxlen = 10;
    for(int i = 0 ; i < num ; i++) {
        auto len = strnlen(names[i]->d_name, 255);
        maxlen = len > maxlen ? len : maxlen;
    }
    struct stat st;
    size_t pathlen = strnlen(path,1024);
    size_t max_path_len = maxlen+pathlen+2; //+1 for / and 1 for \0
    char * fullpath = malloc(sizeof(char)*(max_path_len));
    snprintf(fullpath,pathlen+1,"%s/",path);
    pathlen+=1;
    for( int i = 1 ; i < num; i++) {
        auto color = map_color (names[i]);
        snprintf(&fullpath[pathlen], maxlen, "%s", names[i]->d_name);
        int status = stat(fullpath, &st);
        if (status==-1) {free(names[i]); continue;}
        uint64_t filelen = (uint64_t)st.st_size;
        printf("%s %-*s %luB\n", color, (int)maxlen, names[i]->d_name, (filelen));
        free(names[i]); //needs stdlib.h
    }
    printf("%s",COLOR_RESET);
    closedir(dir);
    free(fullpath);
    free(names[0]);
    free(names);
    return 0;
}
