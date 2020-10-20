#include "lib.h"
#include "types.h"


int main(int argc, char *argv[]) {
    int ret;
    if (argc == 1) {
        printf("%s: missing operand\n", argv[0]);
    }
    for (int i = 1; i < argc; i++) {
        if (is_dir(argv[i]))
            printf("%s: cannot create directory '%s': File exists\n", argv[0], argv[i]);
        else
        {
            ret = open(argv[i], O_CREATE|O_DIRECTORY);
            if (ret < 0)
                printf("%s: cannot create directory '%s': %s\n", argv[0], argv[i], errs[-1 * ret]);
            else
                close(ret);
        }
    }
    exit();
}