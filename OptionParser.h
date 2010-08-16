/**
 * Copyright (C) 2010 Johannes Weißl <jargon@molb.org>
 * License: your favourite BSD-style license
 *
 * git clone http://github.com/weisslj/cpp-optparse.git
 *
 * This is yet another option parser for C++. It is modelled after the
 * excellent Python optparse API. Although incomplete, anyone familiar to
 * optparse should feel at home:
 * http://docs.python.org/library/optparse.html
 *
 * Design decisions:
 * - elegant and easy usage more important than speed / flexibility
 * - shortness more important than feature completeness
 *   * no unicode
 *   * no checking for user programming errors
 *
 * Why not use getopt/getopt_long?
 * - not C++ / not completely POSIX
 * - too cumbersome to use, would need lot of additional code
 *
 * Why not use Boost.Program_options?
 * - boost not installed on all target platforms (esp. cluster, HPC, ...)
 * - too big to include just for option handling:
 *   322 *.h (44750 lines) + 7 *.cpp (2078 lines)
 *
 * Why not use tclap/Opag/Options/CmdLine/Anyoption/Argument_helper/...?
 * - no reason, writing one is faster than code inspection :-)
 * - similarity to Python desired for faster learning curve
 *
 * Future work:
 * - nargs > 1?
 * - comments?
 *
 * Python only features:
 * - conflict handlers
 * - adding new actions
 *
 *
 * Example:
 *
 * using optparse::OptionParser;
 *
 * OptionParser parser = OptionParser() .description("just an example");
 *
 * parser.add_option("-f", "--file") .dest("filename")
 *                   .help("write report to FILE") .metavar("FILE");
 * parser.add_option("-q", "--quiet")
 *                   .action("store_false") .dest("verbose") .set_default("1")
 *                   .help("don't print status messages to stdout");
 * 
 * optparse::Values options = parser.parse_args(argc, argv);
 * vector<string> args = parser.args();
 *
 * if (options.get("verbose"))
 *     cout << options["filename"] << endl;
 *
 */

#ifndef OPTIONPARSER_H_
#define OPTIONPARSER_H_

#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <iostream>
#include <sstream>

#ifdef OS_WINDOWS
// #include <AtlBase.h>
#include <tchar.h>
#endif

namespace optparse {

#if defined(OS_WINDOWS)
#if defined(UNICODE) || defined(_UNICODE)
#define tcout wcout
#define tcerr wcerr
#else
#define tcout cout
#define tcerr cerr
#endif

#elif defined(USE_WCHAR)
typedef wchar_t TCHAR;
#define _T(x) L ## x
#define tcout wcout
#define tcerr wcerr
#define _totupper ::towupper

#else
typedef char TCHAR;
#define _T(x) x
#define tcout cout
#define tcerr cerr
#define _totupper ::toupper

#endif

typedef std::basic_string<TCHAR> tstring;
typedef std::basic_ostream<TCHAR> tostream;
typedef std::basic_ostringstream<TCHAR> tostringstream;
typedef std::basic_istringstream<TCHAR> tistringstream;
typedef std::basic_stringstream<TCHAR> tstringstream;


class OptionParser;
class OptionGroup;
class Option;
class Values;
class Value;
class Callback;

typedef std::map<tstring,tstring> strMap;
typedef std::map<tstring,std::list<tstring> > lstMap;
typedef std::map<tstring,Option const*> optMap;

const TCHAR* const SUPPRESS_HELP = _T("SUPPRESS") _T("HELP");
const TCHAR* const SUPPRESS_USAGE = _T("SUPPRESS") _T("USAGE");

//! Class for automatic conversion from string -> anytype
class Value {
  public:
    Value() : str(), valid(false) {}
    Value(const tstring& v) : str(v), valid(true) {}
    operator const TCHAR*() { return str.c_str(); }
    operator bool() { bool t; return (valid && (tistringstream(str) >> t)) ? t : false; }
    operator short() { short t; return (valid && (tistringstream(str) >> t)) ? t : 0; }
    operator unsigned short() { unsigned short t; return (valid && (tistringstream(str) >> t)) ? t : 0; }
    operator int() { int t; return (valid && (tistringstream(str) >> t)) ? t : 0; }
    operator unsigned int() { unsigned int t; return (valid && (tistringstream(str) >> t)) ? t : 0; }
    operator long() { long t; return (valid && (tistringstream(str) >> t)) ? t : 0; }
    operator unsigned long() { unsigned long t; return (valid && (tistringstream(str) >> t)) ? t : 0; }
    operator float() { float t; return (valid && (tistringstream(str) >> t)) ? t : 0; }
    operator double() { double t; return (valid && (tistringstream(str) >> t)) ? t : 0; }
    operator long double() { long double t; return (valid && (tistringstream(str) >> t)) ? t : 0; }
 private:
    const tstring str;
    bool valid;
};

class Values {
  public:
    Values() : _map() {}
    const tstring& operator[] (const tstring& d) const;
    tstring& operator[] (const tstring& d) { return _map[d]; }
    bool is_set(const tstring& d) const { return _map.find(d) != _map.end(); }
    bool is_set_by_user(const tstring& d) const { return _userSet.find(d) != _userSet.end(); }
    void is_set_by_user(const tstring& d, bool yes);
    Value get(const tstring& d) const { return (is_set(d)) ? Value((*this)[d]) : Value(); }

    typedef std::list<tstring>::iterator iterator;
    typedef std::list<tstring>::const_iterator const_iterator;
    std::list<tstring>& all(const tstring& d) { return _appendMap[d]; }
    const std::list<tstring>& all(const tstring& d) const { return _appendMap.find(d)->second; }

  private:
    strMap _map;
    lstMap _appendMap;
    std::set<tstring> _userSet;
};

class OptionParser {
  public:
    OptionParser();
    virtual ~OptionParser() {}

    OptionParser& usage(const tstring& u) { set_usage(u); return *this; }
    OptionParser& version(const tstring& v) { _version = v; return *this; }
    OptionParser& description(const tstring& d) { _description = d; return *this; }
    OptionParser& add_help_option(bool h) { _add_help_option = h; return *this; }
    OptionParser& add_version_option(bool v) { _add_version_option = v; return *this; }
    OptionParser& prog(const tstring& p) { _prog = p; return *this; }
    OptionParser& epilog(const tstring& e) { _epilog = e; return *this; }
    OptionParser& set_defaults(const tstring& dest, const tstring& val) {
      _defaults[dest] = val; return *this;
    }
    OptionParser& enable_interspersed_args() { _interspersed_args = true; return *this; }
    OptionParser& disable_interspersed_args() { _interspersed_args = false; return *this; }
    OptionParser& add_option_group(const OptionGroup& group);

    const tstring& usage() const { return _usage; }
    const tstring& version() const { return _version; }
    const tstring& description() const { return _description; }
    bool add_help_option() const { return _add_help_option; }
    bool add_version_option() const { return _add_version_option; }
    const tstring& prog() const { return _prog; }
    const tstring& epilog() const { return _epilog; }
    bool interspersed_args() const { return _interspersed_args; }

    Option& add_option(const tstring& opt);
    Option& add_option(const tstring& opt1, const tstring& opt2);
    Option& add_option(const tstring& opt1, const tstring& opt2, const tstring& opt3);
    Option& add_option(const std::vector<tstring>& opt);

    Values& parse_args(int argc, TCHAR const* const* argv);
    Values& parse_args(const std::vector<tstring>& args);
    template<typename InputIterator>
    Values& parse_args(InputIterator begin, InputIterator end) {
      return parse_args(std::vector<tstring>(begin, end));
    }

    const std::list<tstring>& args() const { return _leftover; }
    std::vector<tstring> args() {
      return std::vector<tstring>(_leftover.begin(), _leftover.end());
    }

    tstring format_help() const;
    tstring format_option_help(unsigned int indent = 2) const;
    void print_help() const;

    void set_usage(const tstring& u);
    tstring get_usage() const;
    void print_usage(tostream& out) const;
    void print_usage() const;

    tstring get_version() const;
    void print_version(tostream& out) const;
    void print_version() const;

    void error(const tstring& msg) const;
    void exit() const;

  private:
    const Option& lookup_short_opt(const tstring& opt) const;
    const Option& lookup_long_opt(const tstring& opt) const;

    void handle_short_opt(const tstring& opt, const tstring& arg);
    void handle_long_opt(const tstring& optstr);

    void process_opt(const Option& option, const tstring& opt, const tstring& value);

    tstring format_usage(const tstring& u) const;

    tstring _usage;
    tstring _version;
    tstring _description;
    bool _add_help_option;
    bool _add_version_option;
    tstring _prog;
    tstring _epilog;
    bool _interspersed_args;

    Values _values;

    std::list<Option> _opts;
    optMap _optmap_s;
    optMap _optmap_l;
    strMap _defaults;
    std::list<OptionGroup const*> _groups;

    std::list<tstring> _remaining;
    std::list<tstring> _leftover;
};

class OptionGroup : public OptionParser {
  public:
    OptionGroup(const OptionParser& p, const tstring& t, const tstring& d = tstring()) :
      _parser(p), _title(t), _group_description(d) {}
    virtual ~OptionGroup() {}

    OptionGroup& title(const tstring& t) { _title = t; return *this; }
    OptionGroup& group_description(const tstring& d) { _group_description = d; return *this; }
    const tstring& title() const { return _title; }
    const tstring& group_description() const { return _group_description; }

  private:
    const OptionParser& _parser;
    tstring _title;
    tstring _group_description;
};

class Option {
  public:
    Option() : _action(_T("store")), _type(_T("string")), _nargs(1), _callback(0) {}
    virtual ~Option() {}

    Option& action(const tstring& a);
    Option& type(const tstring& t) { _type = t; return *this; }
    Option& dest(const tstring& d) { _dest = d; return *this; }
    Option& set_default(const tstring& d) { _default = d; return *this; }
    template<typename T>
    Option& set_default(T t) { tostringstream ss; ss << t; _default = ss.str(); return *this; }
    Option& nargs(size_t n) { _nargs = n; return *this; }
    Option& set_const(const tstring& c) { _const = c; return *this; }
    template<typename InputIterator>
    Option& choices(InputIterator begin, InputIterator end) {
      _choices.assign(begin, end); type(_T("choice")); return *this;
    }
    Option& help(const tstring& h) { _help = h; return *this; }
    Option& metavar(const tstring& m) { _metavar = m; return *this; }
    Option& callback(Callback& c) { _callback = &c; return *this; }

    const tstring& action() const { return _action; }
    const tstring& type() const { return _type; }
    const tstring& dest() const { return _dest; }
    const tstring& get_default() const { return _default; }
    size_t nargs() const { return _nargs; }
    const tstring& get_const() const { return _const; }
    const std::list<tstring>& choices() const { return _choices; }
    const tstring& help() const { return _help; }
    const tstring& metavar() const { return _metavar; }
    Callback* callback() const { return _callback; }

  private:
    tstring check_type(const tstring& opt, const tstring& val) const;
    tstring format_option_help(unsigned int indent = 2) const;
    tstring format_help(unsigned int indent = 2) const;

    std::set<tstring> _short_opts;
    std::set<tstring> _long_opts;

    tstring _action;
    tstring _type;
    tstring _dest;
    tstring _default;
    size_t _nargs;
    tstring _const;
    std::list<tstring> _choices;
    tstring _help;
    tstring _metavar;
    Callback* _callback;

    friend class OptionParser;
};

class Callback {
public:
  virtual void operator() (const Option& option, const tstring& opt, const tstring& val, const OptionParser& parser) = 0;
  virtual ~Callback() {}
};

}

#endif
