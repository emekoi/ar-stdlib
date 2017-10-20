#!/usr/bin/python2.7
import os, sys, shutil, platform, time, json
import install

OUTPUT = ""
SUFFIX = ".so"
COMPILER = "gcc"
# INCLUDE = [  ]
# SOURCE = [  ]
FLAGS = [ "-O3" ]
LINK = [ "libaria" ]
# DEFINE = [  ]
# EXTRA = [  ]

if platform.system() == "Windows":
  sys.stdout = os.fdopen(sys.stdout.fileno(), 'w', 0)
  SUFFIX = ".dll"
  LINK += [ "mingw32" ]
  FLAGS += [ "-mwindows" ]


# if platform.system() == "Darwin":

# if platform.system() == "Linux":



def fmt(fmt, dic):
  for k in dic:
    v = " ".join(dic[k]) if type(dic[k]) is list else dic[k]
    fmt = fmt.replace("{" + k + "}", str(dic[k]))
  return fmt


def clearup():
  if os.path.isfile(OUTPUT + SUFFIX):
    os.remove(OUTPUT + SUFFIX)

def main():
  global FLAGS, SOURCE, LINK

  print "initing..."
  SEARCH = [ d + "/package.json" for d in os.listdir(".") if os.path.isdir(d) and d[0] != '.' ]

  starttime = time.time()

  # Handle args
  verbose = "verbose" in sys.argv


  # Make sure there arn't any temp files left over from a previous build
  # clearup()

  for directory in SEARCH:
    config = json.load(open(directory))

    print "building " + config['name'] + SUFFIX

    # Build
    cmd = fmt(
      "%s -o {name}.%s {flags} {src} {include} {link} {define} {extra}", config) % (COMPILER, SUFFIX)

    # if verbose:
    print cmd

    print "compiling..."
    # res = os.system(cmd)
    res = 0
    if os.path.isfile(OUTPUT):
      print "stripping..."
      # os.system("strip %s" % OUTPUT)
    if res == 0:
      print "done (%.2fs)" % (time.time() - starttime)
    else:
      print "done with errors"

  # print "clearing up..."
  # clearup()

  sys.exit(res)


if __name__ == "__main__":
  main()
