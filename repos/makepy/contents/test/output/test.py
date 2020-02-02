import sys

tag = sys.argv[1]

sys.stdout.write("STDOUT-" + tag + "\n")
sys.stderr.write("STDERR-" + tag + "\n")
