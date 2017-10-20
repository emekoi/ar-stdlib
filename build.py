#!/usr/bin/python2.7
import os, sys, shutil, platform, time, json
import install

SUFFIX = ".so"
COMPILER = "gcc"
INCLUDE = [  ]
# LINK = [ "aria" ]
LINK = [ ]
DEFINE = [  ]
FLAGS = [ "-Wall", "-Wextra", "--std=c99", "-pedantic", "-fno-strict-aliasing", "-O3" ]
EXTRA = [  ]

if platform.system() == "Windows":
  SUFFIX = ".dll"
  LINK += [ "mingw32" ]


# if platform.system() == "Darwin":
# if platform.system() == "Linux":


def fmt(fmt, dic):
  for k in dic:
    v = " ".join(dic[k]) if type(dic[k]) is list else dic[k]
    fmt = fmt.replace("{" + k + "}", str(v))
  return fmt


def clearup(file):
  if os.path.isfile(file + SUFFIX):
    os.remove(file + SUFFIX)

def main():
  SEARCH = [ d + "/package.json" for d in os.listdir(".") if os.path.isdir(d) and d[0] != '.' ]

  starttime = time.time()

  # Handle args
  verbose = "verbose" in sys.argv

  for directory in SEARCH:
    config = json.load(open(directory))

    print "building %s..." % (config['name'] + SUFFIX)

    # Build

    src = []
    for file in config['src']:
      src += [ "%s/%s" % (config['name'], file) ]

    cmd = fmt("%s -o {name}%s {flags} {src} " +
      "{include} {link} {define} {extra}",
      {
        "name"    : config['name'],
        "src"     : " ".join(src),
        "include" : " ".join(map(lambda x:"-I" + x, config['include'] + INCLUDE)),
        "link"    : " ".join(map(lambda x:"-l" + x, config['link'] + LINK)),
        "define"  : " ".join(map(lambda x:"-D" + x, config['define'] + DEFINE)),
        "flags"   : " ".join(config['flags'] + FLAGS),
        "extra"   : " ".join(config['extra'] + EXTRA),
      })

    cmd %= (COMPILER, SUFFIX)

    if verbose:
      print cmd

    clearup(config['name'])
    # os.system(cmd)

    if os.path.isfile(config['name'] + SUFFIX):
    #   os.system("strip %s" % (config['name'] + SUFFIX))

  # install.process(SEARCH)

if __name__ == "__main__":
  main()
