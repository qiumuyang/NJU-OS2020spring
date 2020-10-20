#include "lib.h"
#include "types.h"


int main(int argc, char *argv[]) {
    int ret;
    if (argc == 1) {
        printf("%s: missing operand\n", argv[0]);
    }
    for (int i = 1; i < argc; i++) {
        if (is_dir(argv[i]))
            printf("%s: cannot remove '%s': Is a directory\n", argv[0], argv[i]);
        else
        {
            ret = remove(argv[i]);
            if (ret < 0)
                printf("%s: cannot remove '%s': %s\n", argv[0], argv[i], errs[-1 * ret]);
        }
    }
    exit();
}