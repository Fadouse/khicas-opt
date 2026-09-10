#ifndef KHICAS_PARAMETRIC_DISPLAY_H
#define KHICAS_PARAMETRIC_DISPLAY_H
#include "usual.h"
namespace giac {
extern const unary_function_ptr * const at_cart2param;
extern const unary_function_ptr * const at_polar2param;
// Presentation only: never evaluate the labels as calls or replace CAS data.
// Read names from the original call, including constant-coordinate branches.
inline bool parametric_display_view(const gen &request,const gen &result,gen &view){
  const gen *call=&request;
  for(unsigned depth=0;depth<8;++depth){
    if(call->is_symb_of_sommet(at_simplify) || call->is_symb_of_sommet(at_normal) || call->is_symb_of_sommet(at_ratnormal)){
      call=&call->_SYMBptr->feuille;
      if(call->type==_VECT && call->subtype==_SEQ__VECT)return false;
      continue;
    }
    if(call->is_symb_of_sommet(at_sto)){
      const gen &f=call->_SYMBptr->feuille;
      if(f.type!=_VECT || f._VECTptr->size()!=2)return false;
      call=&f._VECTptr->front();continue;
    }
    break;
  }
  bool cart=call->is_symb_of_sommet(at_cart2param);
  if(!cart && !call->is_symb_of_sommet(at_polar2param))return false;
  const gen &f=call->_SYMBptr->feuille;
  if(f.type!=_VECT || f._VECTptr->size()!=3)return false;
  const vecteur &args=*f._VECTptr;
  if(args[1].type!=_VECT || args[1]._VECTptr->size()!=2 || args[2].type!=_IDNT)return false;
  if(result.type!=_VECT || result._VECTptr->empty() || result._VECTptr->size()>16)return false;
  gen x=cart?args[1]._VECTptr->front():gen(identificateur("x"));
  gen y=cart?args[1]._VECTptr->back():gen(identificateur("y"));
  if(x.type!=_IDNT || y.type!=_IDNT)return false;
  gen xcall=symbolic(at_of,makesequence(x,args[2]));
  gen ycall=symbolic(at_of,makesequence(y,args[2]));
  // Labels add bounded wrapper nodes and share coordinate expressions; their
  // construction need not scan or copy a potentially large result subtree.
  // The renderer applies its own allocation limit after adding the labels.
  vecteur branches;
  branches.reserve(result._VECTptr->size());
  for(unsigned i=0;i<result._VECTptr->size();++i){
    const gen &branch=(*result._VECTptr)[i];
    if(branch.type!=_VECT || branch._VECTptr->size()!=2)return false;
    branches.push_back(makevecteur(symb_equal(xcall,branch._VECTptr->front()),symb_equal(ycall,branch._VECTptr->back())));
  }
  view=branches;return true;
}
// A selected rendered cell corresponds to the original coordinate expression.
// Whole-row and whole-result copies remain directly usable as parameter data.
inline gen parametric_display_selection(const gen &result,int row,int column){
  if(result.type!=_VECT || row<0 || unsigned(row)>=result._VECTptr->size())return result;
  const gen &branch=(*result._VECTptr)[row];
  if(branch.type!=_VECT || branch._VECTptr->size()!=2)return result;
  if(column<0 || column>1)return branch;
  return (*branch._VECTptr)[column];
}
}
#endif
