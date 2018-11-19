cc_binary(
    name = "deswear",
    srcs = ["deswearDriver.cpp", "deswear.h"],
    deps = [
        "@libgit2//:libgit2"
    ]
)

cc_test(
    name = "deswear_test",
    srcs = ["automationTest/testDeswear.cpp"],
    deps = [":deswear"]
)