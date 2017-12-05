#!/usr/bin/python2.7
import os, sys, platform, json

INSTALL = "install"
SUFFIX = ".so"

# EXECUTABLE 755
# REGULAR 644

if platform.system() == "Windows":
  sys.stdout = os.fdopen(sys.stdout.fileno(), 'w', 0)
  SUFFIX = ".dll"
  # INSTALL = "cp"


def fmt(fmt, dic):
  for k in dic:
    fmt = fmt.replace("{" + k + "}", str(dic[k]))
  return fmt


def process(packages):
  res = 0

  for package in packages:
    config = json.load(open(package))

    print "installing %s%s" % (config['name'], SUFFIX)

    cmd = fmt("mkdir -p \"/usr/local/{target}/aria/{version}\" && " +
        "%s -m644 {name}%s \"/usr/local/{target}/aria/{version}\"", config) % (INSTALL, SUFFIX)
      

def main():
  if len(sys.argv) < 2:
    print "usage: install MODULE/module.json"
    sys.exit(1)

  process(sys.argv[1:])


if __name__ == "__main__":
  main()