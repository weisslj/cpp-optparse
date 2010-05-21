#include "OptionParser.h"

#include <iostream>
#include <sstream>
#include <string>
#include <complex>

using namespace std;

using optparse::OptionParser;

int main(int argc, char *argv[])
{
  const string usage = "usage: %prog [OPTION]... DIR [FILE]...";
  const string version = "%prog 1.0\nCopyright (C) 2010 Johannes Weißl\n"
    "License GPLv3+: GNU GPL version 3 or later "
    "<http://gnu.org/licenses/gpl.html>.\n"
    "This is free software: you are free to change and redistribute it.\n"
    "There is NO WARRANTY, to the extent permitted by law.";
  const string desc = "Lorem ipsum dolor sit amet, consectetur adipisicing "
    "elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut "
    "enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut "
    "aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in "
    "voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint "
    "occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit "
    "anim id est laborum.";
  const string epilog = "Sed ut perspiciatis unde omnis iste natus error sit"
    "voluptatem accusantium doloremque laudantium, totam rem aperiam, eaque ipsa"
    "quae ab illo inventore veritatis et quasi architecto beatae vitae dicta"
    "sunt explicabo. Nemo enim ipsam voluptatem quia voluptas sit aspernatur aut"
    "odit aut fugit, sed quia consequuntur magni dolores eos qui ratione"
    "voluptatem sequi nesciunt. Neque porro quisquam est, qui dolorem ipsum quia"
    "dolor sit amet, consectetur, adipisci velit, sed quia non numquam eius modi"
    "tempora incidunt ut labore et dolore magnam aliquam quaerat voluptatem.";

  OptionParser parser = OptionParser()
    .usage(usage)
    .version(version)
    .description(desc)
    .epilog(epilog)
  ;

  parser.set_defaults("verbosity", "50");
  parser.set_defaults("no_clear", "0");

  // test all actions
  parser.add_option("--clear") .action("store_true") .dest("no_clear") .help("clear (default)");
  parser.add_option("--no-clear") .action("store_false") .dest("clear") .help("not clear");
  parser.add_option("--string")
    .help("This is a really long text... very long indeed! It must be wrapped on normal terminals.");
  parser.add_option("-x", "--clause", "--sentence") .metavar("SENTENCE") .set_default("I'm a sentence")
    .help("This is a really long text... very long indeed! It must be wrapped on normal terminals. "
          "Also it should appear not on the same line as the option.");
  parser.add_option("-k") .action("count") .help("how many times?");
  parser.add_option("-v", "--verbose") .action("store_const") .set_const("100") .dest("verbosity") .help("be verbose!");
  parser.add_option("-s", "--silent") .action("store_const") .set_const("0") .dest("verbosity") .help("be silent!");
  parser.add_option("-n", "--number") .type("int") .set_default("1") .metavar("NUM") .help("number of files");
  parser.add_option("-H") .action("help") .help("alternative help");
  parser.add_option("-V") .action("version") .help("alternative version");
  parser.add_option("-i", "--int") .action("store") .type("int");
  parser.add_option("-f", "--float") .action("store") .type("float");
  parser.add_option("-c", "--complex") .action("store") .type("complex");
  char const* const choices[] = { "foo", "bar", "baz" };
  parser.add_option("-C", "--choices") .choices(&choices[0], &choices[3]);

  optparse::Values& options = parser.parse_args(argc, argv);
  vector<string> args = parser.args();

  bool no_clear = options.get("no_clear");
  cout << "clear: " << (no_clear ? "true" : "false") << endl;
  cout << "string: " << options["string"] << endl;
  cout << "clause: " << options["clause"] << endl;
  cout << "k: " << options["k"] << endl;
  cout << "verbosity: " << options["verbosity"] << endl;
  cout << "number: " << (int) options.get("number") << endl;
  cout << "int: " << (int) options.get("int") << endl;
  cout << "float: " << (float) options.get("float") << endl;
  complex<double> c = 0;
  if (options.is_set("complex")) {
    stringstream ss;
    ss << options["complex"];
    ss >> c;
  }
  cout << "complex: " << c << endl;
  cout << "choices: " << (const char*) options.get("choices") << endl;

  cout << endl << "leftover arguments: " << endl;
  for (vector<string>::const_iterator it = args.begin(); it != args.end(); ++it) {
    cout << "arg: " << *it << endl;
  }

  return 0;
}
