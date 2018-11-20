#include <algorithm>
#include <stdio.h>
#include <string>
#include <cstring>
#include <array>

#include <iostream>
#include <vector>
#include "git2.h"

const char * COMMIT_MESSAGE = "Whoops";
const std::array<std::string, 6> swears = {{
    " fuck"," shit"," fucked"," ass "," damn"," piss"
}};

void deswear(git_repository* repo);
bool containsSwear(const char * summary);
void updateChildren(git_commit *commit);
void removeSwears(git_repository* repo, git_commit *commit, bool firstCommit);
void rebaseOntoAmended(git_repository* repo, git_oid amendedCommitId, git_oid amendedParentCommidId);

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

    if (firstCommit) {
        git_commit_amend(&oid, commit, "refs/heads/master", NULL, NULL, NULL, COMMIT_MESSAGE, NULL);
    } else {
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

    git_rebase *rebaseObject = NULL;
    
    git_rebase_options rebaseOpt{};
    git_rebase_init_options(&rebaseOpt, GIT_REBASE_OPTIONS_VERSION);
    rebaseOpt.merge_options.file_favor = GIT_MERGE_FILE_FAVOR_UNION;


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

    git_rebase_operation *rebaseOperation = NULL;
    while (git_rebase_next(&rebaseOperation, rebaseObject) != GIT_ITEROVER) {
        git_commit *rebaseCommit;
        git_oid *mergedRebaseCommitId;
        
        if (rebaseOperation != NULL) {
            git_commit_lookup(&rebaseCommit, repo, &rebaseOperation->id);

            // git_rebase_commit(mergedRebaseCommitId, rebaseObject, NULL, git_commit_author(rebaseCommit), NULL, NULL);
            while(git_rebase_commit(mergedRebaseCommitId, rebaseObject, NULL, git_commit_author(rebaseCommit), NULL, NULL) == GIT_EUNMERGED) {
                std::string wait ;
                std::cout << "Please deal with merge conflict then click enter on this page.\n";
                std::cin >> wait; 
                std::cout << "Continueing.\n";
            }

            git_commit_free(rebaseCommit);
            std::string CommitMsg(git_commit_summary(rebaseCommit));
            std::cout << "\r-- Applying commit \"" << git_oid_tostr_s(git_commit_id(rebaseCommit)) << "\"     " << std::flush;

        } else {
            
            std::string CommitMsg(git_commit_summary(rebaseCommit));
            std::cout << "-- Problem at \"" << CommitMsg << "\"" << std::endl;
            std::cout << "-- Problem ID at \"" << git_oid_tostr_s(git_commit_id(rebaseCommit)) << "\"" << std::endl;
            const git_error *e = giterr_last();
            std::cout << "\n\nError: " << error << " / " << e->klass << " : " << e->message << std::endl;
            exit(1);
        }
    }

    error = git_rebase_finish(rebaseObject, NULL);
    if (error < 0){
        const git_error *e = giterr_last();
        std::cout << "Error: " << error << " / " << e->klass << " : " << e->message << std::endl;
        exit(1);
    }

    git_rebase_free(rebaseObject);
    
}