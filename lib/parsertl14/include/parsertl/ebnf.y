/* Generate code using: bison -S parsertl.cc ebnf.y */
%token EMPTY IDENTIFIER PREC TERMINAL
%%

rule: rhs_or;

rhs_or: opt_prec_list
      | rhs_or '|' opt_prec_list;

opt_prec_list: opt_list opt_prec;

opt_list:
        | EMPTY
        | rhs_list;

rhs_list: rhs
        | rhs_list rhs;

rhs: IDENTIFIER
   | TERMINAL
   | '[' rhs_or ']'
   | rhs '?'
   | '{' rhs_or '}'
   | rhs '*'
   | '{' rhs_or '}' '-'
   | rhs '+'
   | '(' rhs_or ')';

opt_prec:
        | PREC IDENTIFIER
        | PREC TERMINAL;

%%
