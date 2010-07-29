/**
 * Copyright (C) 2010 Johannes Weißl <jargon@molb.org>
 * License: your favourite BSD-style license
 *
 * See OptionParser.h for help.
 */

#include "OptionParser.h"

#include <cstdlib>
#include <algorithm>
#include <complex>

#if defined(ENABLE_NLS) && ENABLE_NLS
# include <libintl.h>
# define _(s) gettext(s)
#else
# define _(s) ((const TCHAR *) (s))
#endif

using namespace std;

namespace optparse {

////////// auxiliary (string) functions { //////////
struct str_wrap {
  str_wrap(const tstring& l, const tstring& r) : lwrap(l), rwrap(r) {}
  str_wrap(const tstring& w) : lwrap(w), rwrap(w) {}
  tstring operator() (const tstring& s) { return lwrap + s + rwrap; }
  const tstring lwrap, rwrap;
};
template<typename InputIterator, typename UnaryOperator>
static tstring str_join_trans(const tstring& sep, InputIterator begin, InputIterator end, UnaryOperator op) {
  tstring buf;
  for (InputIterator it = begin; it != end; ++it) {
    if (it != begin)
      buf += sep;
    buf += op(*it);
  }
  return buf;
}
template<class InputIterator>
static tstring str_join(const tstring& sep, InputIterator begin, InputIterator end) {
  return str_join_trans(sep, begin, end, str_wrap(_T("")));
}
static tstring& str_replace(tstring& s, const tstring& patt, const tstring& repl) {
  size_t pos = 0, n = patt.length();
  while (true) {
    pos = s.find(patt, pos);
    if (pos == tstring::npos)
      break;
    s.replace(pos, n, repl);
    pos += repl.size();
  }
  return s;
}
static tstring str_replace(const tstring& s, const tstring& patt, const tstring& repl) {
  tstring tmp = s;
  str_replace(tmp, patt, repl);
  return tmp;
}
static tstring str_format(const tstring& s, size_t pre, size_t len, bool indent_first = true) {
  tstringstream ss;
  tstring p;
  if (indent_first)
    p = tstring(pre, ' ');

  size_t pos = 0, linestart = 0;
  size_t line = 0;
  while (true) {
    bool wrap = false;

    size_t new_pos = s.find_first_of(_T(" \n\t"), pos);
    if (new_pos == tstring::npos)
      break;
    if (s[new_pos] == '\n') {
      pos = new_pos + 1;
      wrap = true;
    }
    if (line == 1)
      p = tstring(pre, ' ');
    if (wrap || new_pos + pre > linestart + len) {
      ss << p << s.substr(linestart, pos - linestart - 1) << endl;
      linestart = pos;
      line++;
    }
    pos = new_pos + 1;
  }
  ss << p << s.substr(linestart) << endl;
  return ss.str();
}
static tstring str_inc(const tstring& s) {
  tstringstream ss;
  tstring v = (s != _T("")) ? s : _T("0");
  long i;
  tistringstream(v) >> i;
  ss << i+1;
  return ss.str();
}
static unsigned int cols() {
  unsigned int n = 80;
#if !defined(OS_WINDOWS) && !defined(USE_WCHAR)
  const TCHAR *s = getenv("COLUMNS");
  if (s)
    tistringstream(s) >> n;
#endif
  return n;
}
static tstring basename(const tstring& s) {
  tstring b = s;
  size_t i = b.find_last_not_of('/');
  if (i == tstring::npos) {
    if (b[0] == '/')
      b.erase(1);
    return b;
  }
  b.erase(i+1, b.length()-i-1);
  i = b.find_last_of(_T("/"));
  if (i != tstring::npos)
    b.erase(0, i+1);
  return b;
}
////////// } auxiliary (string) functions //////////


////////// class OptionParser { //////////
OptionParser::OptionParser() :
  _usage(_(_T("%prog [options]"))),
  _add_help_option(true),
  _add_version_option(true),
  _interspersed_args(true) {}

Option& OptionParser::add_option(const tstring& opt) {
  const tstring tmp[1] = { opt };
  return add_option(vector<tstring>(&tmp[0], &tmp[1]));
}
Option& OptionParser::add_option(const tstring& opt1, const tstring& opt2) {
  const tstring tmp[2] = { opt1, opt2 };
  return add_option(vector<tstring>(&tmp[0], &tmp[2]));
}
Option& OptionParser::add_option(const tstring& opt1, const tstring& opt2, const tstring& opt3) {
  const tstring tmp[3] = { opt1, opt2, opt3 };
  return add_option(vector<tstring>(&tmp[0], &tmp[3]));
}
Option& OptionParser::add_option(const vector<tstring>& v) {
  _opts.resize(_opts.size()+1);
  Option& option = _opts.back();
  tstring dest_fallback;
  for (vector<tstring>::const_iterator it = v.begin(); it != v.end(); ++it) {
    if (it->substr(0,2) == _T("--")) {
      const tstring s = it->substr(2);
      if (option.dest() == _T(""))
        option.dest(str_replace(s, _T("-"), _T("_")));
      option._long_opts.insert(s);
      _optmap_l[s] = &option;
    } else {
      const tstring s = it->substr(1,1);
      if (dest_fallback == _T(""))
        dest_fallback = s;
      option._short_opts.insert(s);
      _optmap_s[s] = &option;
    }
  }
  if (option.dest() == _T(""))
    option.dest(dest_fallback);
  return option;
}

OptionParser& OptionParser::add_option_group(const OptionGroup& group) {
  for (list<Option>::const_iterator oit = group._opts.begin(); oit != group._opts.end(); ++oit) {
    const Option& option = *oit;
    for (set<tstring>::const_iterator it = option._short_opts.begin(); it != option._short_opts.end(); ++it)
      _optmap_s[*it] = &option;
    for (set<tstring>::const_iterator it = option._long_opts.begin(); it != option._long_opts.end(); ++it)
      _optmap_l[*it] = &option;
  }
  _groups.push_back(&group);
  return *this;
}

const Option& OptionParser::lookup_short_opt(const tstring& opt) const {
  optMap::const_iterator it = _optmap_s.find(opt);
  if (it == _optmap_s.end())
    error(_(_T("no such option")) + tstring(_T(": -")) + opt);
  return *it->second;
}

void OptionParser::handle_short_opt(const tstring& opt, const tstring& arg) {

  _remaining.pop_front();
  tstring value;

  const Option& option = lookup_short_opt(opt);
  if (option._nargs == 1) {
    value = arg.substr(2);
    if (value == _T("")) {
      if (_remaining.empty())
        error(_T("-") + opt + _T(" ") + _(_T("option requires an argument")));
      value = _remaining.front();
      _remaining.pop_front();
    }
  } else {
    if (arg.length() > 2)
      _remaining.push_front(tstring(_T("-")) + arg.substr(2));
  }

  process_opt(option, tstring(_T("-")) + opt, value);
}

const Option& OptionParser::lookup_long_opt(const tstring& opt) const {

  list<tstring> matching;
  for (optMap::const_iterator it = _optmap_l.begin(); it != _optmap_l.end(); ++it) {
    if (it->first.compare(0, opt.length(), opt) == 0)
      matching.push_back(it->first);
  }
  if (matching.size() > 1) {
    tstring x = str_join(_T(", "), matching.begin(), matching.end());
    error(_(_T("ambiguous option")) + tstring(_T(": --")) + opt + _T(" (") + x + _T("?)"));
  }
  if (matching.size() == 0)
    error(_(_T("no such option")) + tstring(_T(": --")) + opt);

  return *_optmap_l.find(matching.front())->second;
}

void OptionParser::handle_long_opt(const tstring& optstr) {

  _remaining.pop_front();
  tstring opt, value;

  size_t delim = optstr.find(_T("="));
  if (delim != tstring::npos) {
    opt = optstr.substr(0, delim);
    value = optstr.substr(delim+1);
  } else
    opt = optstr;

  const Option& option = lookup_long_opt(opt);
  if (option._nargs == 1 and delim == tstring::npos) {
    if (not _remaining.empty()) {
      value = _remaining.front();
      _remaining.pop_front();
    }
  }

  if (option._nargs == 1 and value == _T(""))
    error(_T("--") + opt + _T(" ") + _(_T("option requires an argument")));

  process_opt(option, tstring(_T("--")) + opt, value);
}

Values& OptionParser::parse_args(const int argc, TCHAR const* const* const argv) {
  if (prog() == _T(""))
    prog(basename(argv[0]));
  return parse_args(&argv[1], &argv[argc]);
}
Values& OptionParser::parse_args(const vector<tstring>& v) {

  _remaining.assign(v.begin(), v.end());

  if (add_version_option() and version() != _T("")) {
    add_option(_T("--version")) .action(_T("version")) .help(_(_T("show program's version number and exit")));
    _opts.splice(_opts.begin(), _opts, --(_opts.end()));
  }
  if (add_help_option()) {
    add_option(_T("-h"), _T("--help")) .action(_T("help")) .help(_(_T("show this help message and exit")));
    _opts.splice(_opts.begin(), _opts, --(_opts.end()));
  }

  while (not _remaining.empty()) {
    const tstring arg = _remaining.front();

    if (arg == _T("--")) {
      _remaining.pop_front();
      break;
    }

    if (arg.substr(0,2) == _T("--")) {
      handle_long_opt(arg.substr(2));
    } else if (arg.substr(0,1) == _T("-") and arg.length() > 1) {
      handle_short_opt(arg.substr(1,1), arg);
    } else {
      _remaining.pop_front();
      _leftover.push_back(arg);
      if (not interspersed_args())
        break;
    }
  }
  while (not _remaining.empty()) {
    const tstring arg = _remaining.front();
    _remaining.pop_front();
    _leftover.push_back(arg);
  }

  for (strMap::const_iterator it = _defaults.begin(); it != _defaults.end(); ++it) {
    if (not _values.is_set(it->first))
      _values[it->first] = it->second;
  }

  for (list<Option>::const_iterator it = _opts.begin(); it != _opts.end(); ++it) {
    if (it->get_default() != _T("") and not _values.is_set(it->dest()))
        _values[it->dest()] = it->get_default();
  }

  return _values;
}

void OptionParser::process_opt(const Option& o, const tstring& opt, const tstring& value) {
  if (o.action() == _T("store")) {
    tstring err = o.check_type(opt, value);
    if (err != _T(""))
      error(err);
    _values[o.dest()] = value;
    _values.is_set_by_user(o.dest(), true);
  }
  else if (o.action() == _T("store_const")) {
    _values[o.dest()] = o.get_const();
    _values.is_set_by_user(o.dest(), true);
  }
  else if (o.action() == _T("store_true")) {
    _values[o.dest()] = _T("1");
    _values.is_set_by_user(o.dest(), true);
  }
  else if (o.action() == _T("store_false")) {
    _values[o.dest()] = _T("0");
    _values.is_set_by_user(o.dest(), true);
  }
  else if (o.action() == _T("append")) {
    tstring err = o.check_type(opt, value);
    if (err != _T(""))
      error(err);
    _values[o.dest()] = value;
    _values.all(o.dest()).push_back(value);
    _values.is_set_by_user(o.dest(), true);
  }
  else if (o.action() == _T("append_const")) {
    _values[o.dest()] = o.get_const();
    _values.all(o.dest()).push_back(o.get_const());
    _values.is_set_by_user(o.dest(), true);
  }
  else if (o.action() == _T("count")) {
    _values[o.dest()] = str_inc(_values[o.dest()]);
    _values.is_set_by_user(o.dest(), true);
  }
  else if (o.action() == _T("help")) {
    print_help();
    std::exit(0);
  }
  else if (o.action() == _T("version")) {
    print_version();
    std::exit(0);
  }
  else if (o.action() == _T("callback") && o.callback()) {
    (*o.callback())(o, opt, value, *this);
  }
}

tstring OptionParser::format_option_help(unsigned int indent /* = 2 */) const {
  tstringstream ss;

  if (_opts.empty())
    return ss.str();

  for (list<Option>::const_iterator it = _opts.begin(); it != _opts.end(); ++it) {
    if (it->help() != SUPPRESS_HELP)
      ss << it->format_help(indent);
  }

  return ss.str();
}

tstring OptionParser::format_help() const {
  tstringstream ss;

  if (usage() != SUPPRESS_USAGE)
    ss << get_usage() << endl;

  if (description() != _T(""))
    ss << str_format(description(), 0, cols()) << endl;

  ss << _(_T("Options")) << _T(":") << endl;
  ss << format_option_help();

  for (list<OptionGroup const*>::const_iterator it = _groups.begin(); it != _groups.end(); ++it) {
    const OptionGroup& group = **it;
    ss << endl << _T("  ") << group.title() << _T(":") << endl;
    if (group.group_description() != _T(""))
      ss << str_format(group.group_description(), 4, cols()) << endl;
    ss << group.format_option_help(4);
  }

  if (epilog() != _T(""))
    ss << endl << str_format(epilog(), 0, cols());

  return ss.str();
}
void OptionParser::print_help() const {
  tcout << format_help();
}

void OptionParser::set_usage(const tstring& u) {
  tstring lower = u;
  transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
  if (lower.compare(0, 7, _T("usage: ")) == 0)
    _usage = u.substr(7);
  else
    _usage = u;
}
tstring OptionParser::format_usage(const tstring& u) const {
  tstringstream ss;
  ss << _(_T("Usage")) << _T(": ") << u << endl;
  return ss.str();
}
tstring OptionParser::get_usage() const {
  if (usage() == SUPPRESS_USAGE)
    return tstring(_T(""));
  return format_usage(str_replace(usage(), _T("%prog"), prog()));
}
void OptionParser::print_usage(tostream& out) const {
  tstring u = get_usage();
  if (u != _T(""))
    out << u << endl;
}
void OptionParser::print_usage() const {
  print_usage(tcout);
}

tstring OptionParser::get_version() const {
  return str_replace(_version, _T("%prog"), prog());
}
void OptionParser::print_version(tostream& out) const {
  out << get_version() << endl;
}
void OptionParser::print_version() const {
  print_version(tcout);
}

void OptionParser::exit() const {
  std::exit(2);
}
void OptionParser::error(const tstring& msg) const {
  print_usage(tcerr);
  tcerr << prog() << _T(": ") << _(_T("error")) << _T(": ") << msg << endl;
  exit();
}
////////// } class OptionParser //////////

////////// class Values { //////////
const tstring& Values::operator[] (const tstring& d) const {
  strMap::const_iterator it = _map.find(d);
  static const tstring empty = _T("");
  return (it != _map.end()) ? it->second : empty;
}
void Values::is_set_by_user(const tstring& d, bool yes) {
  if (yes)
    _userSet.insert(d);
  else
    _userSet.erase(d);
}
////////// } class Values //////////

////////// class Option { //////////
tstring Option::check_type(const tstring& opt, const tstring& val) const {
  tistringstream ss(val);
  tstringstream err;

  if (type() == _T("int") || type() == _T("long")) {
    long t;
    if (not (ss >> t))
      err << _(_T("option")) << _T(" ") << opt << _T(": ") << _(_T("invalid integer value")) << _T(": '") << val << _T("'");
  }
  else if (type() == _T("float") || type() == _T("double")) {
    double t;
    if (not (ss >> t))
      err << _(_T("option")) << _T(" ") << opt << _T(": ") << _(_T("invalid floating-point value")) << _T(": '") << val << _T("'");
  }
  else if (type() == _T("choice")) {
    if (find(choices().begin(), choices().end(), val) == choices().end()) {
      list<tstring> tmp = choices();
      transform(tmp.begin(), tmp.end(), tmp.begin(), str_wrap(_T("'")));
      err << _(_T("option")) << _T(" ") << opt << _T(": ") << _(_T("invalid choice")) << _T(": '") << val << _T("'")
        << _T(" (") << _(_T("choose from")) << _T(" ") << str_join(_T(", "), tmp.begin(), tmp.end()) << _T(")");
    }
  }
  else if (type() == _T("complex")) {
    complex<double> t;
    if (not (ss >> t))
      err << _(_T("option")) << _T(" ") << opt << _T(": ") << _(_T("invalid complex value")) << _T(": '") << val << _T("'");
  }

  return err.str();
}

tstring Option::format_option_help(unsigned int indent /* = 2 */) const {

  tstring mvar_short, mvar_long;
  if (nargs() == 1) {
    tstring mvar = metavar();
    if (mvar == _T("")) {
      mvar = type();
      transform(mvar.begin(), mvar.end(), mvar.begin(), ::toupper);
     }
    mvar_short = _T(" ") + mvar;
    mvar_long = _T("=") + mvar;
  }

  tstringstream ss;
  ss << tstring(indent, ' ');

  if (not _short_opts.empty()) {
    ss << str_join_trans(_T(", "), _short_opts.begin(), _short_opts.end(), str_wrap(_T("-"), mvar_short));
    if (not _long_opts.empty())
      ss << _T(", ");
  }
  if (not _long_opts.empty())
    ss << str_join_trans(_T(", "), _long_opts.begin(), _long_opts.end(), str_wrap(_T("--"), mvar_long));

  return ss.str();
}

tstring Option::format_help(unsigned int indent /* = 2 */) const {
  tstringstream ss;
  tstring h = format_option_help(indent);
  unsigned int width = cols();
  unsigned int opt_width = min(width*3/10, 36u);
  bool indent_first = false;
  ss << h;
  // if the option list is too long, start a new paragraph
  if (h.length() >= (opt_width-1)) {
    ss << endl;
    indent_first = true;
  } else {
    ss << tstring(opt_width - h.length(), ' ');
    if (help() == _T(""))
      ss << endl;
  }
  if (help() != _T("")) {
    tstring help_str = (get_default() != _T("")) ? str_replace(help(), _T("%default"), get_default()) : help();
    ss << str_format(help_str, opt_width, width, indent_first);
  }
  return ss.str();
}

Option& Option::action(const tstring& a) {
  _action = a;
  if (a == _T("store_const") || a == _T("store_true") || a == _T("store_false") ||
      a == _T("append_const") || a == _T("count") || a == _T("help") || a == _T("version"))
    nargs(0);
  return *this;
}
////////// } class Option //////////

}
