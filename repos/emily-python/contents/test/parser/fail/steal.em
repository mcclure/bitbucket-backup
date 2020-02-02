# Make sure stealing only works across newlines, not commas
# FIXME: Rewrite this test to use --ast2 instead of using execute success/fail
# Expect failure

# This ought to fail, but succeeds, becuase stealing fallaciously works across commas
# Tags: broken-all

if 1 (3), else (4)
