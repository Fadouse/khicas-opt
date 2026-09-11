#ifndef KHICAS_CONDITIONAL_EVAL_H
#define KHICAS_CONDITIONAL_EVAL_H
namespace giac {
// Mathematical when/piecewise keep an equality with free identifiers
// undecided. Structural "same" returning false is not a proof of inequality.
// Program if/else and the standalone == operator keep their usual semantics.
inline bool conditional_symbolic_equal(const gen &test,gen &result,GIAC_CONTEXT){
  if(!(test.is_symb_of_sommet(at_equal) || test.is_symb_of_sommet(at_same)) ||
     test._SYMBptr->feuille.type!=_VECT || test._SYMBptr->feuille._VECTptr->size()!=2 ||
     taille(test,65)>64)return false;
  const vecteur &v=*test._SYMBptr->feuille._VECTptr;
  if(v[0]==v[1])return false;
  vecteur ids=lidnt(test);
  bool variable=false;
  for(unsigned j=0;j<ids.size();++j){
    if(ids[j]==cst_pi)continue;
    // Inspect bindings, not the test's expressions: evaluating a function
    // call here and again in the ordinary selector could repeat side effects.
    gen value=ids[j].eval(1,contextptr);
    vecteur remaining=lidnt(value);
    for(unsigned k=0;k<remaining.size();++k)if(remaining[k]!=cst_pi){variable=true;break;}
    if(variable)break;
  }
  if(!variable)return false;
  result=symb_equal(v[0],v[1]);return true;
}
}
#endif
