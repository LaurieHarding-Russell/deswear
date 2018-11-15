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
git_oid removeSwears(git_repository* repo, git_commit *commit, git_oid *childCommit, bool firstCommit);
void rebaseChild(git_repository* repo, git_oid parentCommitId, git_oid *childCommit);
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
    git_oid childCommitId;
    bool firstCommit = true;
    while(!git_revwalk_next(&oid, walker)) {
		git_commit_lookup(&commit, repo, &oid);

        const char * summary = git_commit_summary(commit);

        if (containsSwear(summary)) {
            std::cout << git_oid_tostr_s(&oid) << " " << git_commit_summary(commit) << std::endl; 
            childCommitId = removeSwears(repo, commit, &childCommitId, firstCommit);
        } else {
            childCommitId = oid;
        }
        
		git_commit_free(commit);
        firstCommit = false;
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

git_oid removeSwears(git_repository* repo, git_commit *commit, git_oid *childCommitId, bool firstCommit) {
    git_oid oid;

    if (firstCommit) {
        git_commit_amend(&oid, commit, "master", NULL, NULL, NULL, COMMIT_MESSAGE, NULL);
    } else {
        git_commit_amend(&oid, commit, NULL, NULL, NULL, NULL, COMMIT_MESSAGE, NULL);
    }
    if (!firstCommit) {
        std::cout << "  -- New Parent " << git_oid_tostr_s(&oid) << std::endl;
        std::cout << "  -- Child " << git_oid_tostr_s(childCommitId) << std::endl;
        rebaseChild(repo, oid, childCommitId);
    }
    return oid;
}

// Rebasing is yucky. FIXME, fix this monster.
void rebaseChild(git_repository* repo, git_oid parentCommitId, git_oid *childCommitId) {

    git_annotated_commit *parentAnnotatedCommit = NULL;
    git_annotated_commit_lookup(&parentAnnotatedCommit, repo, &parentCommitId); 

    git_annotated_commit *childAnnotatedCommit;
    git_annotated_commit_lookup(&childAnnotatedCommit, repo, childCommitId); 

    git_rebase *rebaseObject = NULL;
    git_rebase_operation *rebaseOperationObject = NULL;
    git_rebase_options rebaseOpt = GIT_REBASE_OPTIONS_INIT;
    std::cout << "asdf..." << git_oid_tostr_s(childCommitId) << " \n";


    // Initializing rebase: What we want to do.
    int error = git_rebase_open(&rebaseObject, repo, &rebaseOpt);
    if (error == GIT_ENOTFOUND) {
        if (git_rebase_init(&rebaseObject, repo, childAnnotatedCommit, NULL, parentAnnotatedCommit, &rebaseOpt) != 0) {
            std::cout << "We have a problem..." << git_oid_tostr_s(childCommitId) << " \n";
            exit(1);
        }
    } else if (error > 1){
        const git_error *e = giterr_last();
        std::cout << "Error: " << error << " / " << e->klass << " : " << e->message << std::endl;
        exit(1);
    }

    int entrycount = git_rebase_operation_entrycount(rebaseObject);
    std::cout<< "rebase entry count: " << entrycount << "\n";

    // Rebase next: Performing the actions required to make rebase initialize a thing.
    while (git_rebase_next(&rebaseOperationObject, rebaseObject) != GIT_ITEROVER) {
        uint current = git_rebase_operation_current(rebaseObject);
        std::cout<< "rebase current index: " << current << "\n";
        if (GIT_REBASE_NO_OPERATION == current) {
            std::cout<< "rebase no operation" << "\n";
        }
        rebaseOperationObject = git_rebase_operation_byindex(rebaseObject, 0);
    }

    // Not supporting merging

    // Finished? 
    error = git_rebase_finish(rebaseObject, NULL);
    if (error < 0){
        const git_error *e = giterr_last();
        std::cout << "Error: " << error << " / " << e->klass << " : " << e->message << std::endl;
        exit(1);
    }

    git_rebase_free(rebaseObject);
}

