#include <algorithm>
#include <stdio.h>
#include <string>
#include <cstring>
#include <array>

#include <iostream>
#include <vector>
#include "git2.h"

const char * REPO_LOCATION = "/home/laurie/test";
const char * COMMIT_MESSAGE = "Whoops";

bool containsSwear(const char * summary);
void deswear(git_repository* repo);
void updateChildren(git_commit *commit);
void removeSwears(git_repository* repo, git_commit *commit, bool firstCommit);
void rebaseOntoAmended(git_repository* repo, git_oid amendedCommitId, git_oid amendedParentCommidId);

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
    bool firstCommit = true;
    while(!git_revwalk_next(&oid, walker)) {
		git_commit_lookup(&commit, repo, &oid);

        const char * summary = git_commit_summary(commit);

        if (containsSwear(summary)) {
            std::cout << git_oid_tostr_s(&oid) << " " << git_commit_summary(commit) << std::endl; 
            removeSwears(repo, commit, firstCommit);
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

void removeSwears(git_repository* repo, git_commit *commit, bool firstCommit) {
    git_oid oid;

    git_reference * parent = NULL;
    git_branch_create(&parent, repo, "Amending", commit, true);

    // git_index* out;
    // git_cherrypick_commit(out);

    if (firstCommit) {
        std::cout << "test 1 \n";
        git_commit_amend(&oid, commit, "refs/heads/master", NULL, NULL, NULL, COMMIT_MESSAGE, NULL);
    } else {
        std::cout << "test 2 \n";
        git_commit_amend(&oid, commit, NULL, NULL, NULL, NULL, COMMIT_MESSAGE, NULL);
    }

    git_reference * ammended = NULL;
    git_commit* ammendedCommit = NULL;
    git_commit_lookup(&ammendedCommit, repo, &oid);
    git_branch_create(&ammended, repo, "Amended", ammendedCommit, true);
    rebaseOntoAmended(repo, oid, *git_commit_id(commit));
}

// Rebasing is yucky. FIXME, fix this monster.
void rebaseOntoAmended(git_repository* repo, git_oid amendedCommitId, git_oid amendedParentCommidId) {

    git_annotated_commit *amendedAnnotatedCommit = NULL;
    git_reference *refAmended = NULL;
    git_reference_dwim(&refAmended, repo, "Amended");
    git_annotated_commit_from_ref(&amendedAnnotatedCommit, repo, refAmended);

    git_annotated_commit* amendingAnnotatedCommit = NULL; 
    git_reference *refAmending = NULL;
    git_reference_dwim(&refAmending, repo, "Amending");
    git_annotated_commit_from_ref(&amendingAnnotatedCommit, repo, refAmending);

    git_annotated_commit* masterAnnotatedCommit = NULL; 
    git_reference *refMaster = NULL;
    git_reference_dwim(& refMaster, repo, "refs/heads/master");
    git_annotated_commit_from_ref(&masterAnnotatedCommit, repo, refMaster);

    std::cout << git_oid_tostr_s(git_annotated_commit_id(masterAnnotatedCommit)) << " master before" << std::endl; 
    std::cout << git_oid_tostr_s(git_annotated_commit_id(amendedAnnotatedCommit)) << " amended before" << std::endl; 
    std::cout << git_oid_tostr_s(git_annotated_commit_id(amendingAnnotatedCommit)) << " amending before" << std::endl; 

    git_rebase *rebaseObject = NULL;
    git_rebase_operation *rebaseOperationObject = NULL;
    git_rebase_options rebaseOpt = GIT_REBASE_OPTIONS_INIT;

    int error = git_rebase_open(&rebaseObject, repo, &rebaseOpt);
    if (error == GIT_ENOTFOUND) {
        // git rebase --onto Amended Amending master
        if (git_rebase_init(&rebaseObject, repo, masterAnnotatedCommit, amendingAnnotatedCommit, amendedAnnotatedCommit, &rebaseOpt) != 0) {
            std::cout << "We have a problem... \n";
            const git_error *e = giterr_last();
            std::cout << "Error: " << error << " / " << e->klass << " : " << e->message << std::endl;
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

    error = git_rebase_finish(rebaseObject, NULL);
    if (error < 0){
        const git_error *e = giterr_last();
        std::cout << "Error: " << error << " / " << e->klass << " : " << e->message << std::endl;
        exit(1);
    }

    git_rebase_free(rebaseObject);
    
    std::cout << git_oid_tostr_s(git_annotated_commit_id(masterAnnotatedCommit)) << " master after" << std::endl; 
    std::cout << git_oid_tostr_s(git_annotated_commit_id(amendedAnnotatedCommit)) << " amended after" << std::endl; 
    std::cout << git_oid_tostr_s(git_annotated_commit_id(amendingAnnotatedCommit)) << " amending after" << std::endl; 
}