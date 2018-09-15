%token EMPTY IDENTIFIER PREC TERMINAL '|' '[' ']' '?' '{' '}' '*' '-' '+' '(' ')'
%%

rule: rhs_or;

rhs_or: opt_list
      | rhs_or '|' opt_list;

opt_list:
        | EMPTY
        | rhs_list opt_prec;

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
