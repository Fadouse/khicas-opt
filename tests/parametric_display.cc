#include "giacPCH.h"
#include "parametric_display.h"
#include <cassert>
#include <iostream>
namespace giac {gen _cart2param(const gen &,GIAC_CONTEXT);gen _polar2param(const gen &,GIAC_CONTEXT);}
int main(){using namespace giac;context c;const context *ctx=&c;angle_radian(true,ctx);
const char *cases[]={"(x^3+y^3=3*x*y,[x,y],t)","(u=v^2,[u,v],s)","(x^2=4,[x,y],t)","((x^2+y^2)^2=0,[x,y],t)","(rho^3=sin(phi),[rho,phi],s)","((x-2)^2+(y+3)^2=25,[x,y],tau)","(x^2-y^2=1,[x,y],v)","(3*x^2+2*x*y+5*y^2=1,[x,y],q)"};
for(unsigned i=0;i<sizeof(cases)/sizeof(*cases);++i){
 gen args=gen(cases[i],ctx).eval(1,ctx),raw=i==4?_polar2param(args,ctx):_cart2param(args,ctx),saved=raw;
 gen call=symbolic(i==4?at_polar2param:at_cart2param,args),view;
 assert(parametric_display_view(call,raw,view));assert(raw==saved);
 assert(view.type==_VECT && view._VECTptr->size()==raw._VECTptr->size());
 for(unsigned row=0;row<raw._VECTptr->size();++row){
  assert(parametric_display_selection(raw,row,-1)==(*raw._VECTptr)[row]);
  for(unsigned col=0;col<2;++col){
   const gen &eq=(*(*view._VECTptr)[row]._VECTptr)[col];assert(eq.is_symb_of_sommet(at_equal));
   const vecteur &sides=*eq._SYMBptr->feuille._VECTptr;
   assert(sides[1]==(*(*raw._VECTptr)[row]._VECTptr)[col]);
   assert(parametric_display_selection(raw,row,col)==sides[1]);
   assert(sides[0].is_symb_of_sommet(at_of));
   const vecteur &label=*sides[0]._SYMBptr->feuille._VECTptr;
   assert(label[1]==(*args._VECTptr)[2]);
   assert(label[0]==(i==4?gen(identificateur(col?"y":"x")):(*(*args._VECTptr)[1]._VECTptr)[col]));
  }
 }
 assert(parametric_display_selection(raw,-1,-1)==raw);
 if(i==1){
  gen plotted=_plotparam(makesequence(raw._VECTptr->front(),symb_equal((*args._VECTptr)[2],symb_interval(-1,1))),ctx);
  assert(!is_undef(plotted));assert(raw==saved);
 }
 gen nested=symbolic(at_sto,makesequence(gen(symbolic(at_simplify,call)),gen(identificateur("b")))),wrapped;
 assert(parametric_display_view(nested,raw,wrapped));assert(wrapped==view);
 assert(!parametric_display_view(gen(identificateur("b")),raw,wrapped));
 assert(parametric_display_view(symbolic(at_normal,call),raw,wrapped));assert(wrapped==view);
 assert(parametric_display_view(symbolic(at_ratnormal,call),raw,wrapped));assert(wrapped==view);
 std::cout<<view<<'\n';
}
// Constant values still use call names, and labels are never evaluated even
// if a coordinate acquires a value after the conversion finished.
gen args=gen("(u=v,[u,v],s)",ctx),call=symbolic(at_cart2param,args),raw=gen("[[0,0]]",ctx),view;
sto(123,gen(identificateur("u")),ctx);assert(parametric_display_view(call,raw,view));
assert(view._VECTptr->front()._VECTptr->front()._SYMBptr->feuille._VECTptr->front()._SYMBptr->feuille._VECTptr->front()==gen(identificateur("u")));
assert(!parametric_display_view(call,gen("[1,2]",ctx),view));
assert(!parametric_display_view(call,gen(vecteur(17,raw._VECTptr->front())),view));
// Large coordinate trees need no traversal/copy merely to add two labels.
// The UI renderer's separate allocation budget remains unchanged.
vecteur terms;for(int i=1;i<=100;++i)terms.push_back(symbolic(at_sin,i*gen(identificateur("s"))));
gen large=symbolic(at_plus,terms),large_raw=vecteur(1,makevecteur(large,0)),large_view;
assert(taille(large_raw,257)>256);assert(parametric_display_view(call,large_raw,large_view));
const gen &coordinate=large_view._VECTptr->front()._VECTptr->front()._SYMBptr->feuille._VECTptr->back();
assert(coordinate._SYMBptr==large._SYMBptr);assert(large_raw._VECTptr->front()._VECTptr->front()._SYMBptr==large._SYMBptr);
gen wrong=symbolic(at_plus,makesequence(call,1));assert(!parametric_display_view(wrong,raw,view));
for(int i=0;i<9;++i)call=symbolic(at_simplify,call);
assert(!parametric_display_view(call,raw,view));
std::cout<<"PASS: labelled views, custom names, multiple branches, constants, wrappers, raw selection and bounded rejection\n";
}
