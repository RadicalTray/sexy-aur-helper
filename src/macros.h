#define EXECVP(file, args, ...) do {\
    pid_t pid;\
    if ((pid=fork()) == 0) {\
        execvp(file, args);\
        perror("execvp");\
        exit(EXIT_FAILURE);\
    } else if (pid < 0) {\
        perror("fork");\
    } else {\
        int ret = waitpid(pid, NULL, 0);\
        if (ret == -1) {\
            perror("waitpid");\
            exit(EXIT_FAILURE);\
        }\
    }} while (0)


#define SAFE_MKDIR(path) do {\
    struct stat s;\
    const int stat_ret = stat(path, &s);\
    if (stat_ret == -1) {\
        if (errno == ENOENT) {\
            if (mkdir(path, S_IRWXU) != 0) {\
                perror("mkdir");\
            }\
        } else {\
            perror("stat");\
            exit(EXIT_FAILURE);\
        }\
    } else {\
        if (!S_ISDIR(s.st_mode)) {\
            fprintf(stderr, "%s already exists, but is not a directory!", path);\
            exit(EXIT_FAILURE);\
        }\
    } } while (0)
