#include <algorithm>
#include <stdio.h>
#include <string>
#include <cstring>
#include <array>

#include <iostream>
#include "deswear.h"

const char * REPO_LOCATION = "/home/laurie/ISC-ORT";

int main() {
    git_libgit2_init();

    git_repository *repo = NULL;

    int error = git_repository_open(&repo, REPO_LOCATION);
    if (error < 0) {    
        const git_error *e = giterr_last();
        printf("Error %d/%d: %s\n", error, e->klass, e->message);
        exit(error);
    }

    deswear(repo);

    git_libgit2_shutdown();
    return 0;
}