load("@bazel_tools//tools/build_defs/repo:git.bzl", "new_git_repository")

new_git_repository(
    name = "libgit2",
    build_file = "BUILD.libgit2",
    remote = "https://github.com/libgit2/libgit2",
    # version 0.27.* uses .h.in I would have to make a custom rule to make it work.
    # version 0.25+ have st_ctime_nsec which doesn't work
    tag = "v0.24.6",
)