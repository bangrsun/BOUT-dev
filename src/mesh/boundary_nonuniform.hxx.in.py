#!/usr/bin/env python3

from jinja2 import Environment

env=Environment(trim_blocks=True);


orders=range(1,5)
whats=["Dirichlet","Neumann","Free"]

class_str="""
#include "boundary_op.hxx"

class {{class}} : public BoundaryOp {
public:
  {{class}}() {}
  {{class}}(BoundaryRegion *region, std::shared_ptr<FieldGenerator> gen = nullptr) : BoundaryOp(region), gen(gen) {}
  BoundaryOp *clone(BoundaryRegion *region, const list<string> &args) override;

  using BoundaryOp::apply;
  void apply(Field2D &f) override {
    throw BoutException("Not Implemented");
  };
 
  void apply(Field3D &f) override {
    apply(f,0.0);
  };
  void apply(Field3D &f, BoutReal t) override;
  
private:
  std::shared_ptr<FieldGenerator>  gen; // Generator
  void calc_interp_to_stencil(
{% for i in range(order) %}BoutReal x{{i}}, {% endfor %}
{% for i in range(order) %}BoutReal &fac{{i}}{% if loop.last %}){% else %}, {% endif %}{% endfor %} const ;
};
"""


if __name__ == "__main__":
    for order in orders:
        for what in whats:
            if what == "Neumann" and order == 1:
                continue
            args={
                'order':order,
                'what':what,
                'class':"Boundary%sNonUniform_O%d"%(what,order),
            }
            print(env.from_string(class_str).render(**args))