#include "lib.h"
#include "types.h"


int main(int argc, char *argv[]) {
    uint8_t buf[128];
    int ret;
    if (argc <= 2) {
        printf("%s: missing operand\n", argv[0]);
    }
    else if (argc > 3) {
        printf("%s: too many arguments\n", argv[0]);
    }
    else {
        if (is_dir(argv[1]))
            printf("%s: cannot copy '%s': Is a directory\n", argv[0], argv[1]);
        else
        {
            int src = open(argv[1], O_READ);
            if (src < 0)
                printf("%s: cannot copy '%s': %s\n", argv[0], argv[1], errs[-1 * src]);
            else {
                int dst = open(argv[2], O_CREATE|O_TRUNC|O_WRITE);
                if (dst < 0)
                    printf("%s: cannot copy to '%s': %s\n", argv[0], argv[2], errs[-1 * dst]);
                else
                {
                    do {
                        ret = read(src, buf, 128);
                        write(dst, buf, ret);
                    } while (ret == 128);
                    close(dst);
                }
                close(src);
            }
        }
    }
    exit();
}