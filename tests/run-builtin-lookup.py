#!/usr/bin/env python3
"""Keep builtin lookup names, aliases, tokens and initialization behavior unchanged."""

from repository import source_path
from pathlib import Path
import sys, re, subprocess, tempfile, json, argparse
from integration_build import ROOT as root, function

p = argparse.ArgumentParser(
    description="Compare complete lexer lookup bodies using actual registration tables and instrumented pointer/token fixtures."
)
p.add_argument("--output", type=Path)
args = p.parse_args()
baseline = function(
    subprocess.check_output(["git", "show", "dff6974:kglobal.cc"], cwd=root, text=True),
    "    int find_or_make_symbol(",
).replace("find_or_make_symbol(", "before_find_or_make_symbol(", 1)
candidate = function(
    source_path("kglobal.cc").read_text(), "    int find_or_make_symbol("
).replace("find_or_make_symbol(", "after_find_or_make_symbol(", 1)
pointers = sorted(
    set(re.findall(r"^at_\w+", (source_path("static_lexer_.h")).read_text(), re.M))
)
text = r"""
#include <algorithm>
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <new>
using namespace std; namespace ustl=std;
static bool count_allocations=false;static unsigned allocations=0;
void *operator new(size_t n){if(count_allocations)++allocations;void *p=malloc(n);if(!p)throw bad_alloc();return p;}
void operator delete(void *p) noexcept {free(p);}
#define GIAC_CONTEXT const context *contextptr
#define T_UNARY_OP 265
#define T_SYMBOL 259
#define T_TO 310
struct context { mutable int status=0;};
int &index_status(const context *p){return p->status;}
struct unary_function_ptr {uintptr_t marker;};
static void (*on_pointer_construction)()=0;
struct gen {uintptr_t _FUNC_; int subtype,type;string symbol;
 gen(int n=0):_FUNC_(n),subtype(0),type(0){}
 gen(const unary_function_ptr *p):_FUNC_(reinterpret_cast<uintptr_t>(p)),subtype(1),type(13){if(on_pointer_construction){auto hook=on_pointer_construction;on_pointer_construction=0;hook();}}
};
gen identificateur(const string &s){gen r;r.type=6;r.symbol=s;return r;}
const gen plus_one=1;
struct charptr_gen_unary{const char *s;uintptr_t _FUNC_;unsigned short reserved;signed char subtype;unsigned char type;};
const charptr_gen_unary builtin_lexer_functions[]={
#include "static_lexer.h"
};
using charptr_gen=pair<const char*,gen>;
vector<charptr_gen> make_metadata(){vector<charptr_gen> v;for(auto &b:builtin_lexer_functions){gen g;g._FUNC_=b._FUNC_;g.subtype=b.subtype;g.type=b.type;v.emplace_back(b.s,g);}return v;}
vector<charptr_gen> metadata=make_metadata();
charptr_gen *builtin_lexer_functions_begin(){return metadata.data();}
charptr_gen *builtin_lexer_functions_end(){return metadata.data()+metadata.size();}
bool tri(const charptr_gen &a,const charptr_gen &b){return strcmp(a.first,b.first)<0;}
struct lexer_tab_int_type{const char *keyword;unsigned char status;int value;int subtype;short return_value;};
const lexer_tab_int_type lexer_tab_int_values[]={{"False",1,0,7,260},{"True",1,1,7,260},{"while",0,0,0,287}};
const lexer_tab_int_type *const lexer_tab_int_values_end=lexer_tab_int_values+3;
bool tri1(const lexer_tab_int_type &a,const lexer_tab_int_type &b){return strcmp(a.keyword,b.keyword)<0;}
using sym_string_tab=map<string,gen>;sym_string_tab symbols;
sym_string_tab &syms(){return symbols;}int lock_syms_mutex(){return 0;}void unlock_syms_mutex(){}
"""
for i, p in enumerate(pointers):
    text += f"const unary_function_ptr object_{p}={{{i}}};const unary_function_ptr *const {p}=&object_{p};\n"
text += baseline + candidate
text += r"""
const unary_function_ptr * const expected_pointers[]={
#include "static_lexer_.h"
};
static context ctx;static int nested_calls=0;
void nested_lookup(){gen r;if(after_find_or_make_symbol("evalf",r,0,false,&ctx)!=T_UNARY_OP||r._FUNC_!=reinterpret_cast<uintptr_t>(at_evalf))abort();++nested_calls;}
void equal(const string &s){context a,b;gen old,newer;int ta=before_find_or_make_symbol(s,old,0,false,&a),tb=after_find_or_make_symbol(s,newer,0,false,&b);if(ta!=tb||old._FUNC_!=newer._FUNC_||old.subtype!=newer.subtype||old.type!=newer.type||old.symbol!=newer.symbol||a.status!=b.status){cerr<<"DIFFERENCE "<<s<<endl;exit(1);}}
int main(){
 // First candidate lookup is reentered during the selected pointer conversion.
 on_pointer_construction=nested_lookup;gen r;int first=after_find_or_make_symbol("sin",r,0,false,&ctx);if(nested_calls!=1||first!=T_UNARY_OP||r._FUNC_!=reinterpret_cast<uintptr_t>(at_sin))abort();
 for(size_t i=0;i<metadata.size();++i){equal(metadata[i].first);context c;gen g;int token=after_find_or_make_symbol(metadata[i].first,g,0,false,&c);auto &m=builtin_lexer_functions[i];int expected=m.subtype+(m.subtype<0?512:256);if(token!=expected||g._FUNC_!=reinterpret_cast<uintptr_t>(expected_pointers[i])+(m._FUNC_%2))abort();}
 for(const char *s:{"False","True","while","","a_new_symbol","a_very_long_new_symbol_to_test_lifetime_without_copy"}){equal(s);equal(s);}
 const char *longest="";for(auto &p:metadata)if(strlen(p.first)>strlen(longest))longest=p.first;const string s(longest);context c;
 allocations=0;count_allocations=true;before_find_or_make_symbol(s,r,0,false,&c);count_allocations=false;unsigned before=allocations;
 allocations=0;count_allocations=true;after_find_or_make_symbol(s,r,0,false,&c);count_allocations=false;unsigned after=allocations;
 if(!before||after)abort();
 cout<<"{\"builtin_count\":"<<metadata.size()<<",\"all_tokens_aliases_identical\":true,\"first_call_reentry\":true,\"keyword_symbol_cases\":12,\"long_name_allocations_before\":"<<before<<",\"long_name_allocations_after\":"<<after<<"}"<<endl;
}
"""
with tempfile.TemporaryDirectory(prefix="khicas-builtin-lookup-") as tmp:
    d = Path(tmp)
    (d / "lookup.cc").write_text(text)
    rows = []
    for release in [False, True]:
        exe = d / ("release" if release else "debug")
        subprocess.run(
            [
                "c++",
                "-std=c++11",
                "-O2",
                "-fno-threadsafe-statics",
                "-iquote",
                str(source_path("static_lexer.h").parent),
                str(d / "lookup.cc"),
                "-o",
                str(exe),
            ]
            + (["-DRELEASE"] if release else []),
            check=True,
        )
        out = subprocess.check_output([str(exe)], text=True)
        j = json.loads(out)
        j["release"] = release
        rows.append(j)
        print(out, end="")
    if args.output:
        args.output.write_text(
            json.dumps(
                {
                    "scope": "exact original/candidate lookup bodies, repository builtin tables; mocked gen/pointer objects and three keyword fixtures, independent of host Giac implementation",
                    "cases": rows,
                },
                indent=2,
            )
            + "\n"
        )
