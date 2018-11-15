#include <algorithm>
#include <stdio.h>
#include <string>
#include <cstring>
#include <array>

#include <iostream>
#include "git2.h"

const char * REPO_LOCATION = "/home/laurie/test";
const char * COMMIT_MESSAGE = "Whoops";

bool containsSwear(const char * summary);
void deswear(git_repository* repo);
void updateChildren(git_commit *commit);
void removeSwears(git_repository* repo, git_commit *commit, git_commit *childCommit);
void rebaseChild(git_repository* repo, git_oid parentCommitId, git_commit *childCommit);
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

void deswear(git_repository* repo) {
    git_oid oid;
    git_revwalk *walker;
    git_revwalk_new(&walker, repo);
    git_revwalk_push_head(walker);
    git_commit *commit = NULL;
    git_commit *childCommit = NULL;
    while(!git_revwalk_next(&oid, walker)) {
        if (commit != NULL){
            childCommit = commit;
        }
		git_commit_lookup(&commit, repo, &oid);

        const char * summary = git_commit_summary(commit);

        if (containsSwear(summary)) {
            std::cout << git_oid_tostr_s(&oid) << " " << git_commit_summary(commit) << std::endl; 
            removeSwears(repo, commit, childCommit);
        }
        
		git_commit_free(commit);
    }

    git_revwalk_free(walker);
    git_repository_free(repo);
}

bool containsSwear(const char * summary) {
    std::array<std::string, 3> swears = {{
        "shit", "damn", "fuck"
    }};

    std::string commitMessage = summary;

    std::transform(commitMessage.begin(), commitMessage.end(), commitMessage.begin(), ::tolower);

    for (uint i = 0; i != swears.size(); i++) {
        if(std::strstr(commitMessage.c_str(), swears[i].c_str()) != NULL) {
            return true;
        }
    }
    return false;
}

void removeSwears(git_repository* repo, git_commit *commit, git_commit *childCommit) {
    git_oid oid;
    git_commit_amend(&oid, commit, NULL, NULL, NULL, NULL, COMMIT_MESSAGE, NULL);
    std::cout << "  2. " << git_oid_tostr_s(&oid) << std::endl;
    if (childCommit != NULL) {
        rebaseChild(repo, oid, childCommit);
    }
}

// Rebasing is yucky.
void rebaseChild(git_repository* repo, git_oid parentCommitId, git_commit *childCommit) {

    git_annotated_commit *parentAnnotatedCommit = NULL;
    git_annotated_commit_lookup(&parentAnnotatedCommit, repo, &parentCommitId); 

    const git_oid* childCommitId = git_commit_id(childCommit);

    git_annotated_commit *childAnnotatedCommit;
    git_annotated_commit_lookup(&childAnnotatedCommit, repo, childCommitId); 

    git_rebase *rebaseObject = NULL;
    git_rebase_operation *rebaseOperationObject = NULL;
    if (git_rebase_init(&rebaseObject, repo, childAnnotatedCommit, NULL, parentAnnotatedCommit, NULL) != 0) {
        std::cout << "We have a problem..." << git_oid_tostr_s(childCommitId) << " \n";
        exit(1);
    }
    if (git_rebase_next(&rebaseOperationObject, rebaseObject) != 0) {
        std::cout << "We have a next problem... Rebase failed? \n";
        exit(1);
    }
    git_rebase_free(rebaseObject);
}

