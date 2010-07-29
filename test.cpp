#include "OptionParser.h"

#include <iostream>
#include <sstream>
#include <string>
#include <complex>
#include <algorithm>
#include <cstdlib>

using namespace std;

using namespace optparse;

class Output {
public:
  Output(const tstring& d) : delim(d), first(true) {}
  void operator() (const tstring& s) {
    if (first)
      first = false;
    else
      tcout << delim;
    tcout << s;
  }
  ~Output() { tcout << endl; }
  const tstring& delim;
  bool first;
};

class MyCallback : public optparse::Callback {
public:
  MyCallback() : counter(0) {}
  void operator() (const Option& option, const tstring& opt, const tstring& val, const OptionParser& parser) {
    counter++;
    tcout << _T("--- MyCallback --- ") << counter << _T(". time called") << endl;
    tcout << _T("--- MyCallback --- option.action(): ") << option.action() << endl;
    tcout << _T("--- MyCallback --- opt: ") << opt << endl;
    tcout << _T("--- MyCallback --- val: ") << val << endl;
    tcout << _T("--- MyCallback --- parser.usage(): ") << parser.usage() << endl;
    tcout << endl;
  }
  int counter;
};

int _tmain(int argc, TCHAR **argv)
{
#ifndef DISABLE_USAGE
  const tstring usage = _T("usage: %prog [OPTION]... DIR [FILE]...");
#else
  const tstring usage = SUPPRESS_USAGE;
#endif
  const tstring version = _T("%prog 1.0\nCopyright (C) 2010 Johannes Weißl\n")
    _T("License GPLv3+: GNU GPL version 3 or later ")
    _T("<http://gnu.org/licenses/gpl.html>.\n")
    _T("This is free software: you are free to change and redistribute it.\n")
    _T("There is NO WARRANTY, to the extent permitted by law.");
  const tstring desc = _T("Lorem ipsum dolor sit amet, consectetur adipisicing")
    _T(" elit, sed do eiusmod tempor incididunt ut labore et dolore magna")
    _T(" aliqua.\nUt enim ad minim veniam, quis nostrud exercitation ullamco")
    _T(" laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor")
    _T(" in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla")
    _T(" pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa")
    _T(" qui officia deserunt mollit anim id est laborum.");
  const tstring epilog = _T("Sed ut perspiciatis unde omnis iste natus error sit")
    _T(" voluptatem accusantium doloremque laudantium, totam rem aperiam, eaque")
    _T(" ipsa quae ab illo inventore veritatis et quasi architecto beatae vitae")
    _T(" dicta sunt explicabo. Nemo enim ipsam voluptatem quia voluptas sit")
    _T(" aspernatur aut odit aut fugit, sed quia consequuntur magni dolores eos")
    _T(" qui ratione voluptatem sequi nesciunt. Neque porro quisquam est, qui")
    _T(" dolorem ipsum quia dolor sit amet, consectetur, adipisci velit, sed quia")
    _T(" non numquam eius modi tempora incidunt ut labore et dolore magnam")
    _T(" aliquam quaerat voluptatem.");

  OptionParser parser = OptionParser()
    .usage(usage)
    .version(version)
    .description(desc)
    .epilog(epilog)
#ifdef DISABLE_INTERSPERSED_ARGS
    .disable_interspersed_args()
#endif
  ;

  parser.set_defaults(_T("verbosity"), _T("50"));
  parser.set_defaults(_T("no_clear"), _T("0"));

  // test all actions
  parser.add_option(_T("--clear")) .action(_T("store_false")) .dest(_T("no_clear")) .help(_T("clear (default)"));
  parser.add_option(_T("--no-clear")) .action(_T("store_true")) .help(_T("not clear"));
  parser.add_option(_T("--string"))
    .help(_T("This is a really long text... very long indeed! It must be wrapped on normal terminals."));
  parser.add_option(_T("-x"), _T("--clause"), _T("--sentence")) .metavar(_T("SENTENCE")) .set_default(_T("I'm a sentence"))
    .help(_T("This is a really long text... very long indeed! It must be wrapped on normal terminals. ")
          _T("Also it should appear not on the same line as the option."));
  parser.add_option(_T("-k")) .action(_T("count")) .help(_T("how many times?"));
  parser.add_option(_T("-v"), _T("--verbose")) .action(_T("store_const")) .set_const(_T("100")) .dest(_T("verbosity")) .help(_T("be verbose!"));
  parser.add_option(_T("-s"), _T("--silent")) .action(_T("store_const")) .set_const(_T("0")) .dest(_T("verbosity")) .help(_T("be silent!"));
  parser.add_option(_T("-n"), _T("--number")) .type(_T("int")) .set_default(_T("1")) .metavar(_T("NUM")) .help(_T("number of files (default: %default)"));
  parser.add_option(_T("-H")) .action(_T("help")) .help(_T("alternative help"));
  parser.add_option(_T("-V")) .action(_T("version")) .help(_T("alternative version"));
  parser.add_option(_T("-i"), _T("--int")) .action(_T("store")) .type(_T("int")) .set_default(3) .help(_T("default: %default"));
  parser.add_option(_T("-f"), _T("--float")) .action(_T("store")) .type(_T("float")) .set_default(5.3) .help(_T("default: %default"));
  parser.add_option(_T("-c"), _T("--complex")) .action(_T("store")) .type(_T("complex"));
  TCHAR const* const choices[] = { _T("foo"), _T("bar"), _T("baz") };
  parser.add_option(_T("-C"), _T("--choices")) .choices(&choices[0], &choices[3]);
  parser.add_option(_T("-m"), _T("--more")) .action(_T("append"));
  parser.add_option(_T("--more-milk")) .action(_T("append_const")) .set_const(_T("milk"));
  parser.add_option(_T("--hidden")) .help(SUPPRESS_HELP);

  MyCallback mc;
  parser.add_option(_T("-K"), _T("--callback")) .action(_T("callback")) .callback(mc) .help(_T("callback test"));

  OptionGroup group = OptionGroup(parser, _T("Dangerous Options"),
      _T("Caution: use these options at your own risk. ")
      _T("It is believed that some of them bite."));
  group.add_option(_T("-g")) .action(_T("store_true")) .help(_T("Group option.")) .set_default(_T("0"));
  parser.add_option_group(group);

  Values& options = parser.parse_args(argc, argv);
  vector<tstring> args = parser.args();

  tcout << _T("clear: ") << (options.get(_T("no_clear")) ? _T("false") : _T("true")) << endl;
  tcout << _T("string: ") << options[_T("string")] << endl;
  tcout << _T("clause: ") << options[_T("clause")] << endl;
  tcout << _T("k: ") << options[_T("k")] << endl;
  tcout << _T("verbosity: ") << options[_T("verbosity")] << endl;
  tcout << _T("number: ") << (int) options.get(_T("number")) << endl;
  tcout << _T("int: ") << (int) options.get(_T("int")) << endl;
  tcout << _T("float: ") << (float) options.get(_T("float")) << endl;
  complex<double> c = 0;
  if (options.is_set(_T("complex"))) {
    tstringstream ss;
    ss << options[_T("complex")];
    ss >> c;
  }
  tcout << _T("complex: ") << c << endl;
  tcout << _T("choices: ") << (const TCHAR*) options.get(_T("choices")) << endl;
  tcout << _T("more: ");
  for_each(options.all(_T("more")).begin(), options.all(_T("more")).end(), Output(_T(", ")));
  tcout << _T("more_milk: ");
  {
    Output out(_T(", "));
    for (Values::iterator it = options.all(_T("more_milk")).begin(); it != options.all(_T("more_milk")).end(); ++it)
      out(*it);
  }
  tcout << _T("hidden: ") << options[_T("hidden")] << endl;
  tcout << _T("group: ") << (options.get(_T("g")) ? _T("true") : _T("false")) << endl;

  tcout << endl << _T("leftover arguments: ") << endl;
  for (vector<tstring>::const_iterator it = args.begin(); it != args.end(); ++it) {
    tcout << _T("arg: ") << *it << endl;
  }

  return 0;
}

#if !defined(OS_WINDOWS)
int main(int argc, char *argv[])
{
  TCHAR **targv = NULL;
#if defined(USE_WCHAR)
  targv = (TCHAR **) malloc((argc + 1) * sizeof(TCHAR *));
  for (int i = 0; i < argc; ++i) {
    size_t len = mbstowcs(NULL, argv[i], 0);
    targv[i] = (TCHAR *) malloc((len + 1) * sizeof(TCHAR));
    mbstowcs(targv[i], argv[i], len);
  }
#else
  targv = argv;
#endif

  int retval = _tmain(argc, targv);

#if defined(USE_WCHAR)
  for (int i = 0; i < argc; ++i)
    free(targv[i]);
  free(targv);
#endif

  return retval;
}
#endif
