/* Copyright © 2017 Zandr Martin
   Copyright © 2019-2020 Mario Aichinger <aichingm@gmailcom>

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. */
#include <git2.h>
#include <string.h>
#include <stdio.h>

/* --- raw ansi colors -------------------------------- */
/*
   #define COLOR_BLACK        "\x1b[30m"
   #define COLOR_RED          "\x1b[31m"
   #define COLOR_GREEN        "\x1b[32m"
   #define COLOR_YELLOW       "\x1b[33m"
   #define COLOR_BLUE         "\x1b[34m"
   #define COLOR_MAGENTA      "\x1b[35m"
   #define COLOR_CYAN         "\x1b[36m"
   #define COLOR_BLACK_BOLD   "\x1b[30;1m"
   #define COLOR_RED_BOLD     "\x1b[31;1m"
   #define COLOR_GREEN_BOLD   "\x1b[32;1m"
   #define COLOR_YELLOW_BOLD  "\x1b[33;1m"
   #define COLOR_BLUE_BOLD    "\x1b[34;1m"
   #define COLOR_MAGENTA_BOLD "\x1b[35;1m"
   #define COLOR_CYAN_BOLD    "\x1b[36;1m"
   #define COLOR_RESET        "\x1b[0m"
*/

/* --- zsh specific color formatting ------------------ */
#define COLOR_BLACK_BOLD   "%%{\x1b[30;1m%%}"
#define COLOR_RED_BOLD     "%%{\x1b[31;1m%%}"
#define COLOR_GREEN_BOLD   "%%{\x1b[32;1m%%}"
#define COLOR_YELLOW_BOLD  "%%{\x1b[33;1m%%}"
#define COLOR_BLUE_BOLD    "%%{\x1b[34;1m%%}"
#define COLOR_MAGENTA_BOLD "%%{\x1b[35;1m%%}"
#define COLOR_CYAN_BOLD    "%%{\x1b[36;1m%%}"
#define COLOR_WHITE_BOLD   "%%{\x1b[37;1m%%}"
#define COLOR_RESET        "%%{\x1b[0m%%}"

/* --- config section --------------------------------- */
#define STATUS_PREFIX "("
#define STATUS_SUFFIX ")"
#define STATUS_SEPARATOR "|"
#define BRANCH_NAME_COLOR COLOR_MAGENTA_BOLD
#define STAGED_COLOR COLOR_RED_BOLD
#define STAGED_SYMBOL "●"
#define CONFLICTS_COLOR COLOR_YELLOW_BOLD
#define CONFLICTS_SYMBOL "⭍"
#define CHANGED_COLOR COLOR_BLUE_BOLD
#define CHANGED_SYMBOL "✚"
#define BEHIND_COLOR COLOR_RED_BOLD
#define BEHIND_SYMBOL "↓"
#define AHEAD_COLOR COLOR_CYAN_BOLD
#define AHEAD_SYMBOL "↑"
#define UNTRACKED_COLOR COLOR_WHITE_BOLD
#define UNTRACKED_SYMBOL "…"
#define CLEAN_COLOR COLOR_GREEN_BOLD
#define CLEAN_SYMBOL "✔"
/* --- end config section ----------------------------- */


#define psize_t(x) ((unsigned long) x)
#define BRANCH_NAME_LEN 256

struct status_counts {
    size_t untracked;
    size_t conflicts;
    size_t changed;
    size_t staged;
};

int branch_name(git_repository *repo, char *name) {

    git_reference *ref;

    if (git_reference_lookup(&ref, repo, "HEAD") != 0) {
        if (git_repository_head(&ref, repo) != 0) {
            git_reference_free(ref);
            return 1;
        }
    }

    if (git_reference_type(ref) == GIT_REF_OID) {
        int32_t length = 7;
        git_config *config;
        git_repository_config(&config, repo);
        if (git_config_get_int32(&length, config, "core.abbrev") != 0) {
            length = 7;
        }
        git_config_free(config);

        length += 2;

        *name = ':';
        strncpy(name + 1, git_oid_tostr_s(git_reference_target(ref)), length);
        name[length] = '\0';
    } else {
        const char *tmp = git_reference_symbolic_target(ref);

        if (strchr(tmp, '/')) {
            strncpy(name, strrchr(tmp, '/') + 1, BRANCH_NAME_LEN - 1);
            name[BRANCH_NAME_LEN - 1] = '\0';
        }
    }

    return !*name;
}

int ahead_behind(git_repository *repo, size_t *ahead, size_t *behind) {

    const git_oid *loc_id;
    const git_oid *upst_id;
    git_reference *local = NULL;
    git_reference *upstream = NULL;

    *ahead = 0;
    *behind = 0;

    if (git_repository_head(&local, repo) != 0) {
        if (local != NULL) {
            git_reference_free(local);
        }
        return 1;
    }

    if (git_branch_upstream(&upstream, local) != 0) {
        git_reference_free(local);
        if (upstream != NULL) {
            git_reference_free(upstream);
        }
        return 1;
    }

    loc_id = git_reference_target(local);
    upst_id = git_reference_target(upstream);

    git_graph_ahead_behind(ahead, behind, repo, loc_id, upst_id);

    git_reference_free(local);
    git_reference_free(upstream);
    return 0;
}

int status_cb(const char *path, unsigned int flags, void *payload) {

    struct status_counts *status = (struct status_counts *)payload;

    if (flags & GIT_STATUS_IGNORED) {
        return 0;
    }
     /*printf("%s\n", path);*/
    /*printf("%s delta_no_mod %d, current %d, conflict %d, i_new %d, i_mod %d, i_del%d, i_ren %d, i_typ %d, w_mod %d, w_del %d, w_ren %d,  w_typ %d\n",
        path,
        (flags & GIT_DELTA_UNMODIFIED)>=1,
        (flags == GIT_STATUS_CURRENT),
        (flags & GIT_STATUS_CONFLICTED)>=1,
        (flags&GIT_STATUS_INDEX_NEW)>=1,
        (flags&GIT_STATUS_INDEX_MODIFIED)>=1,
        (flags&GIT_STATUS_INDEX_DELETED)>=1,
        (flags&GIT_STATUS_INDEX_RENAMED)>=1,
        (flags&GIT_STATUS_INDEX_TYPECHANGE)>=1,
        (flags&GIT_STATUS_WT_MODIFIED)>=1,
        (flags&GIT_STATUS_WT_DELETED)>=1,
        (flags&GIT_STATUS_WT_RENAMED)>=1,
        (flags&GIT_STATUS_WT_TYPECHANGE)>=1
    );*/

    /*printf("%s ingr %d, i_new %d, i_mod %d, i_del%d, i_ren %d, i_typ %d, w_new %d, w_mod %d, w_del %d, w_ren %d,  w_typ %d\n",
        path,
        (flags & GIT_STATUS_IGNORED)>1,
        (flags&GIT_STATUS_INDEX_NEW)>1,
        (flags&GIT_STATUS_INDEX_MODIFIED)>1,
        (flags&GIT_STATUS_INDEX_DELETED)>1,
        (flags&GIT_STATUS_INDEX_RENAMED)>1,
        (flags&GIT_STATUS_INDEX_TYPECHANGE)>1,
        (flags&GIT_STATUS_WT_NEW)>1,
        (flags&GIT_STATUS_WT_MODIFIED)>1,
        (flags&GIT_STATUS_WT_DELETED)>1,
        (flags&GIT_STATUS_WT_RENAMED)>1,
        (flags&GIT_STATUS_WT_TYPECHANGE)>1
    );*/ 

    if (flags & (GIT_STATUS_INDEX_NEW |
                 GIT_STATUS_INDEX_MODIFIED |
                 GIT_STATUS_INDEX_DELETED |
                 GIT_STATUS_INDEX_RENAMED |
                 GIT_STATUS_INDEX_TYPECHANGE)) {
        status->staged++;
    }

    if (flags & GIT_STATUS_CONFLICTED) {
        status->conflicts++;
        status->changed++;
    }

    if (flags & (
        GIT_STATUS_WT_MODIFIED |
        GIT_STATUS_WT_DELETED |
        GIT_STATUS_WT_RENAMED |
        GIT_STATUS_WT_TYPECHANGE)) {
        status->changed++;
    }

    if (flags & GIT_STATUS_WT_NEW) {
        status->untracked++;
    }

    return 0;
}

int main() {

    size_t ahead;
    size_t behind;
    char name[BRANCH_NAME_LEN];
    git_buf buf = {0};
    git_repository *repo = NULL;
    git_status_options opts = GIT_STATUS_OPTIONS_INIT;

    struct status_counts status;
    status.untracked = 0;
    status.conflicts = 0;
    status.changed = 0;
    status.staged = 0;


    git_libgit2_init();

    if (git_repository_discover(&buf, ".", 1, "/") != 0) {
        goto cleanup;
    }

    if (git_repository_open(&repo, (&buf)->ptr) != 0) {
        goto cleanup;
    }
<<<<<<< HEAD:gitprompt.c
    /* check https://github.com/libgit2/libgit2/blob/master/tests/status/renames.c#L571 for list of options */
    opts.flags |= GIT_STATUS_OPT_RENAMES_HEAD_TO_INDEX;
    opts.flags |= GIT_STATUS_OPT_RENAMES_INDEX_TO_WORKDIR;
    opts.flags |= GIT_STATUS_OPT_RENAMES_FROM_REWRITES;
    opts.flags |= GIT_STATUS_OPT_INCLUDE_UNTRACKED;
    opts.flags |= GIT_STATUS_OPT_RECURSE_UNTRACKED_DIRS;
=======

    opts.flags |= GIT_STATUS_OPT_RENAMES_HEAD_TO_INDEX|GIT_STATUS_OPT_INCLUDE_UNTRACKED;
>>>>>>> e8f61e0afa8c7f7f63358933ea44ab6c841dc554:git_status_prompt.c
    git_status_foreach_ext(repo, &opts, status_cb, &status);

    ahead_behind(repo, &ahead, &behind);

    if (branch_name(repo, name) != 0) {
        goto cleanup;
    }

    printf(STATUS_PREFIX BRANCH_NAME_COLOR "%s" COLOR_RESET, name);

    if (behind > 0) {
        printf(BEHIND_COLOR BEHIND_SYMBOL "%ld" COLOR_RESET, psize_t(behind));
    }

    if (ahead > 0) {
        printf(AHEAD_COLOR AHEAD_SYMBOL "%ld" COLOR_RESET, psize_t(ahead));
    }

    printf(STATUS_SEPARATOR);

    if (status.staged > 0) {
        printf(STAGED_COLOR STAGED_SYMBOL "%ld" COLOR_RESET, psize_t(status.staged));
    }

    if (status.conflicts > 0) {
        printf(CONFLICTS_COLOR CONFLICTS_SYMBOL "%ld" COLOR_RESET, psize_t(status.conflicts));
    }

    if (status.changed > 0) {
        printf(CHANGED_COLOR CHANGED_SYMBOL "%ld" COLOR_RESET, psize_t(status.changed));
    }

    if (status.untracked > 0) {
        printf(UNTRACKED_COLOR UNTRACKED_SYMBOL "%ld" COLOR_RESET, psize_t(status.untracked));
    }

    if ((status.staged + status.conflicts + status.changed + status.untracked) == 0) {
        printf(CLEAN_COLOR CLEAN_SYMBOL COLOR_RESET);
    }

    printf(STATUS_SUFFIX "\n");

cleanup:
    git_buf_free(&buf);
    git_repository_free(repo);
    git_libgit2_shutdown();
    return 0;
}
