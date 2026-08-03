#include "eval.h"
#include <math.h>

// State for one established WITH-RESTART, reached via INVOKE-RESTART.
// GC-allocated (not stack-local) so nothing ever takes the address of a C
// stack variable -- the only real safety requirement for longjmp is that
// the C stack frame which called setjmp still be on the call stack when
// longjmp fires, which is guaranteed here by construction: a restart_frame
// only becomes reachable via the fresh env cell WITH-RESTART builds for its
// own body, and that env cell is never stored anywhere outside this call's
// dynamic extent (never leaked to a caller, never mutated onto shared
// state), so INVOKE-RESTART can only ever find one whose establishing
// WITH-RESTART is still active.
typedef struct restart_frame {
  jmp_buf buf;
  void* value;
} restart_frame;

// How the "args" list passed to apply_callable should be treated.
typedef enum {
  ARGS_RAW_EVAL,   // args are unevaluated forms; evaluate each in callEnv before binding
  ARGS_VALUES,     // args are already-evaluated values; bind as-is
  ARGS_RAW_NOEVAL  // args are unevaluated forms; bind them AS FORMS, no evaluation at all
} ArgMode;

// The single place a callable (TYPE_LAMBDA, TYPE_MACRO, or TYPE_NATIVE) gets
// applied to its arguments. callEnv is the environment raw argument forms
// are evaluated in (ARGS_RAW_EVAL only); env is the environment used to
// build the callee's closure chain. Keeping them separate means evaluating
// arguments and constructing a closure never get conflated. Whether the
// CALLER should treat the result as a value or as an expansion form to
// eval() again is decided by the caller (via which argMode it passes),
// since deciding "is this a macro" has to happen before argument forms are
// evaluated at all -- this function itself is agnostic to that distinction,
// it just binds args to params either way, using the identical storage
// shape TYPE_LAMBDA and TYPE_MACRO share.
static void* apply_callable(void* callee, void* args, ArgMode argMode, void* callEnv, void* env) {

  switch(get_type(callee)) {

  case TYPE_LAMBDA:
  case TYPE_MACRO:
    {
      void* lambda = callee;
      void* closure = car(lambda);
      void* lambdaArgs = car(cdr(lambda));
      void* vals = args;

      // newenv is always a FRESH cons distinct from env in both branches
      // below, so nothing here ever mutates the caller's env object -- no
      // save/restore needed, and no risk of leaking a pushed frame if a
      // future non-local exit (e.g. a restart) ever unwinds through this
      // frame.
      void* newenv = NULL;
      void* l = NULL;
      if(closure) {
	l = last(closure);
	cdr(l) = car(car(env));
	newenv = cons(l, car(env));
      }
      else {
	newenv = cons(car(env), cdr(env));
      }

      if(!lambdaArgs && vals) return ERROR("Sent args to a function and accepts none!");

      void* nextFrame = NULL;

      if(car(lambdaArgs)) {
	void* i = lambdaArgs;
	for(; is_cons(i); i=cdr(i)) {

	  if(!vals) {
	    return ERROR("NOT ENOUGH ARGUMENTS FOR THE FUNCTION!!!");
	  }

	  void* val = (argMode == ARGS_RAW_EVAL) ? eval(car(vals), callEnv) : car(vals);
	  nextFrame = cons(cons(car(i), val), nextFrame);
	  vals = cdr(vals);
	}

	if(i) {
	  // Dotted param list -- i is the rest-parameter symbol (not NULL,
	  // not a cons). Bind it to everything left in vals, as a list.
	  void* rest = NULL;
	  void* restLast = NULL;
	  for(; vals; vals=cdr(vals)) {
	    void* val = (argMode == ARGS_RAW_EVAL) ? eval(car(vals), callEnv) : car(vals);
	    if(!rest) { rest = cons(val, NULL); restLast = rest; }
	    else { cdr(restLast) = cons(val, NULL); restLast = cdr(restLast); }
	  }
	  nextFrame = cons(cons(i, rest), nextFrame);
	}
	else if(vals) {
	  return ERROR("TOO MANY ARGUMENTS FOR THE FUNCTION!!!");
	}

	// set the new env with the lambda list...
	car(newenv) = cons(nextFrame, car(newenv));
      }
      else if(vals) {
	return ERROR("TOO MANY ARGUMENTS FOR THE FUNCTION!!!");
      }

      // run the code with the new env
      void* ret = NULL;
      for(void* i=cdr(cdr(lambda)); i; i=cdr(i)) {

	ret = eval(car(i), newenv);

	if(is_error(ret)) {
	  return ret;
	}
      }

      // reset the end so we don't mess up the closure.
      if(closure) {
	cdr(l) = NULL;
      }

      return ret;
    }

  case TYPE_NATIVE:
    {
      native_func f = to_pointer(callee)->p;

      if(argMode != ARGS_RAW_EVAL) {
	return f(args, env);
      }

      void* ret = NULL;
      void* last = NULL;
      for(void* i=args; i; i=cdr(i)) {

	void* val = eval(car(i), callEnv);

	if(is_error(val)) {
	  return val;
	}

	if(!ret) {
	  ret = cons(val, NULL);
	  last = ret;
	}
	else {
	  cdr(last) = cons(val, NULL);
	  last = cdr(last);
	}
      }

      return f(ret, env);
    }

  case TYPE_NATIVE_INT:
    {
      // Built-in operators (+, CAR, PRINT, ...) are TYPE_NATIVE_INT values,
      // not TYPE_LAMBDA/TYPE_NATIVE -- eval_list's big NATIVE_INT switch is
      // the only place that knows how to run them, and every one of its
      // ~40 cases hand-rolls its own eval(car(...), env) on its raw operand
      // forms. Rather than touch every case, re-enter that switch but wrap
      // each final VALUE in a QUOTE first for any type eval() would
      // otherwise treat specially (symbols, cons cells, the quote-family
      // types) -- QUOTE's eval() just returns car(list) unevaluated, so
      // this makes the switch's own eval() a no-op on values that are
      // already final, without touching ~40 case bodies. Self-evaluating
      // types (numbers, strings, TRUE, lambdas, ...) pass through unwrapped
      // since a second eval() of those is already a no-op. NULL is never
      // wrapped -- QUOTE on NULL is nonsensical here and NULL is always
      // self-evaluating anyway.
      //
      // If argMode is ARGS_RAW_EVAL, args are still raw forms at this point
      // (mirroring TYPE_NATIVE's own handling just above) and must be
      // evaluated in callEnv first to get final values; otherwise they're
      // already final values (ARGS_VALUES/ARGS_RAW_NOEVAL).
      void* wrapped = NULL;
      void* last = NULL;
      for(void* i=args; i; i=cdr(i)) {

	void* val = (argMode == ARGS_RAW_EVAL) ? eval(car(i), callEnv) : car(i);
	if(is_error(val)) return val;

	switch(val ? get_type(val) : TYPE_NULL) {
	case TYPE_SYMBOL:
	case TYPE_CONS:
	case TYPE_QUOTE:
	case TYPE_BACKTICK:
	case TYPE_COMMA:
	case TYPE_SPLICE:
	  val = create_quotetype(TYPE_QUOTE, val);
	  break;
	default:
	  break;
	}

	if(!wrapped) {
	  wrapped = cons(val, NULL);
	  last = wrapped;
	}
	else {
	  cdr(last) = cons(val, NULL);
	  last = cdr(last);
	}
      }

      return eval_list(cons(callee, wrapped), env);
    }

  default:
    return ERROR("Cannot apply: not a function!");
  }
}

// Public wrapper around apply_callable for callers outside eval.c that
// only ever need "call this with already-evaluated values" -- currently
// just rb-tree.c's custom-comparator support. Keeps ArgMode/apply_callable
// itself internal to eval.c.
void* apply_with_values(void* callee, void* args, void* env) {
  return apply_callable(callee, args, ARGS_VALUES, env, env);
}

// depth counts how many enclosing backticks a COMMA/SPLICE still needs to
// unwind through before it's allowed to fire. The outer TYPE_BACKTICK call
// site enters at depth 1 (one backtick to unwind). A COMMA/SPLICE only
// evaluates when depth == 1 (it belongs to the innermost/only backtick);
// at depth > 1 it re-wraps itself one level out (preserving the marker for
// an enclosing quasiquote to see later) instead of evaluating early --
// this is what makes nested quasiquotes (`` `(a `(b ,c)) ``) work.
void* quasiquote(void* list, void* env, int depth) {

  switch(get_type(list)) {

  case TYPE_CONS:
    if(car(list) && get_type(car(list)) == TYPE_SPLICE && depth == 1) {

      void* head = eval(car(car(list)), env);
      if(is_error(head)) return head;

      if(head != NULL && !is_cons(head)) {
	return ERROR("SPLICE: value is not a list!");
      }

      void* rest = quasiquote(cdr(list), env, depth);
      if(is_error(rest)) return rest;

      if(head == NULL) return rest;

      return append(head, rest);
    }
    else {
      void* carResult = quasiquote(car(list), env, depth);
      if(is_error(carResult)) return carResult;

      void* cdrResult = quasiquote(cdr(list), env, depth);
      if(is_error(cdrResult)) return cdrResult;

      return cons(carResult, cdrResult);
    }
    break;

  case TYPE_COMMA:

    if(depth == 0) {
      return ERROR("COMMA: COMMA MUST BE IN A LIST!!!");
    }
    else if(depth == 1) {
      return eval(car(list), env);
    }
    else {
      return create_quotetype(TYPE_COMMA,
			      quasiquote(car(list), env, depth - 1));
    }
    break;

  case TYPE_BACKTICK:
    // A BACKTICK reached here is always a NESTED one (the outermost
    // backtick is stripped by eval_raw's entry point before quasiquote is
    // ever called) -- it must be re-wrapped, not just unwrapped, since it
    // remains quoted data (its own marker) until a matching outer
    // quasiquote actually evaluates down to it.
    if(depth == 0) {
      return ERROR("BACKTICK: BACKTICK MUST BE IN A LIST!!!");
    }
    else {
      return create_quotetype(TYPE_BACKTICK,
			      quasiquote(car(list), env, depth + 1));
    }
    break;

  case TYPE_SPLICE:
    // Reached only when a SPLICE appears somewhere the TYPE_CONS case
    // above didn't catch it (not a list-head position) -- at depth <= 1
    // there's no "rest of list" to splice into, so it's an error same as
    // depth == 0. At depth > 1 it re-wraps for an enclosing quasiquote,
    // same as TYPE_COMMA.
    if(depth <= 1) {
      return ERROR("SPLICE: SPLICE MUST BE IN A LIST!!!");
    }
    else {
      return create_quotetype(TYPE_SPLICE,
			      quasiquote(car(list), env, depth - 1));
    }
    break;

  default:
    return list;
    break;
  }

  return list;
}

// The real evaluator. Never collapses a TYPE_VALUES result -- that's
// eval()'s job (see below). NTHVALUE/MULTIPLEVALUELIST call this directly
// for their inner expression so they see an uncollapsed TYPE_VALUES.
static void* eval_raw(void* list, void* env) {

  ValueType type = get_type(list);

  /* printf("eval type found as: %s\n", return_type_c_string(list)); */
  /* print(stdout, list, 10); */
  /* printf("\n"); */
  
  switch(type) {

  case TYPE_CONS:
    {
      void* p = eval_list(list, env);
      
      return p;
    }
    
  case TYPE_QUOTE:
    
    if(!car(list)) {
      return ERROR("Error: quote requires something after it!\n");
    }
    
    return car(list);
    break;

  case TYPE_BACKTICK:
    if(!car(list)) {
      return ERROR("Error: quasiquote requires something after it!\n");
    }

    return quasiquote(car(list), env, 1);
    break;
    
  case TYPE_SPLICE:
    return ERROR("Error: Splice must be used inside a QUASIQUOTE!");
    break;
    
  case TYPE_COMMA:
    return ERROR("Error: Comma must be used inside a QUASIQUOTE!");
    break;
    
  case TYPE_SYMBOL:
    {
      // I think this should be my problem!!!!!
      // I need to treat this like a (symbol item item...)
      // Reuse the native int function as a template
      
      for(void* i=car(env); i; i=cdr(i)) {

	switch(get_type(car(i))) {

	case TYPE_RB_TREE:
	  {
	    // Use mapget_pair (returns the (key . value) pair), not mapget
	    // (returns the value alone) -- a variable legitimately bound to
	    // NULL must still be found here. Checking mapget's return value
	    // for truthiness can't tell "found, bound to NULL" apart from
	    // "not found", and would wrongly fall through to the next frame.
	    void* pair = mapget_pair(car(i), list, env);
	    if(is_error(pair)) return pair;
	    if(pair) return cdr(pair);
	  }
	  break;

	case TYPE_CONS:
	  {
	    void* tmp = assoc(list, car(i));
	    if(tmp) {
	      return cdr(tmp);
	    }
	  }
	  break;
	  
	default:
	  ERROR("Found an issue with the environment!!!\n");
	  break;
	}
      }

      return ERROR("Could not find symbol!");  
    }
    break;
      
    
  default:
    return list;
  }

  return list;
}

// Public evaluator. Identical to eval_raw, except a TYPE_VALUES result
// collapses to its first value -- so VALUES works transparently everywhere
// except inside NTHVALUE/MULTIPLEVALUELIST, which call eval_raw directly
// on their inner expression to see the uncollapsed result.
void* eval(void* list, void* env) {
  void* ret = eval_raw(list, env);

  if(ret && get_type(ret) == TYPE_VALUES) {
    // car(ret) is the wrapped VALUES list; its own car is the first value.
    return car(car(ret));
  }

  return ret;
}

void* eval_list(void* list, void* env) {
  
  void* o = car(list);
  ValueType type = get_type(o);
  /* printf("eval_list = "); */
  /* printf("\nType = %i or %s\n", type, return_type_c_string(o)); */
  /* print(stdout, list, 10); */
  /* printf("\n"); */
   
  switch(type) {
  case TYPE_CONS:
    {
      void* f = eval(car(list), env);
      if(is_error(f)) return f;

      if(is_type(f, TYPE_MACRO)) {
	void* expansion = apply_callable(f, cdr(list), ARGS_RAW_NOEVAL, env, env);
	if(is_error(expansion)) return expansion;
	return eval(expansion, env);
      }

      return apply_callable(f, cdr(list), ARGS_RAW_EVAL, env, env);
    }
    break;

  case TYPE_NATIVE_INT:

    switch(to_char(o)->c) {
      
    case N_CONS:
      {
	if(!cdr(list) || !cdr(cdr(list))) {
	  return ERROR("CONS requires TWO arguments!");
	}
	
	void* tmp = cdr(list);
	void* a = eval(car(tmp), env);
	void* b = NULL;

	tmp = cdr(tmp);
	if(tmp) {
	  b = eval(car(tmp), env);
	}

	return cons(a, b);
      }
      break;

    case N_CAR:
      {
	if(!cdr(list) || !car(cdr(list))) {
	  return ERROR("CONS requires ONE arguments!");
	}

	void* target = car(cdr(list));
	target = eval(target, env);

	if(!is_cons(target)) {
	  return ERROR("CONS only works on CONS TYPE!");
	}
	return car(target);
      }
      break;
      
    case N_CDR:
      {
	if(!cdr(list) || !car(cdr(list))) {
	  return ERROR("CDR: Requires one argument!!!");
	}

	void* target = car(cdr(list)); 
	target = eval(target, env);

	if(!is_cons(target)) {
	  return ERROR("ERROR: CAR only works on cons_cells!");
	}

	return cdr(target);
      }
      break;
      
    case N_LIST:
      {
	cc ret = NULL;
	cc next = NULL;

	for(cc i=cdr(list); i; i=cdr(i)) {
	  void* tmp = eval(car(i), env);

	  if(ret) {
	    cc c = cons(tmp, NULL);
	    next->cdr = c;
	    next = c;
	  }
	  else {
	  
	    ret = cons(tmp, NULL);
	    next = ret;
	  }
	}
	
	return ret;
      }
      break;
  
    case N_IF:
      {
	void* pred = cdr(list);
	void* truth = cdr(cdr(list));
	
	if(!pred) {
	  return ERROR("ERROR: nothing to IF!");
	}

	if(!truth) {
	  return ERROR("Nothing to execute in IF statement!\n");
	}
	
	void* predicate = eval(car(pred), env);
		
	if(is_true(predicate)) {
	  return eval(car(truth), env);
	}
	else {
	  void* falsehood = cdr(truth);
	  if(falsehood) {
	    return eval(car(falsehood), env);
	  }
	  else {
	    return NULL;
	  }
	}
      }
      break;

      case N_NIF:
      {
	void* pred = cdr(list);
	void* truth = cdr(cdr(list));
	
	if(!pred) {
	  return ERROR("ERROR: nothing to !?!");
	}

	if(!truth) {
	  return ERROR("Nothing to execute in !? statement!\n");
	}
	
	void* predicate = eval(car(pred), env);

	if(!is_true(predicate)) {
	  return eval_list(car(truth), env);
	}

	return NULL;
	
      }
      break;

      case N_WHEN:
      {
	void* pred = cdr(list);
	void* truth = cdr(cdr(list));
	
	if(!pred) {
	  return ERROR("ERROR: nothing to \?\?!");
	}

	if(!truth) {
	  return ERROR("Nothing to execute in ?? statement!\n");
	}
	
	void* predicate = eval(car(pred), env);
	
	if(is_true(predicate)) {
	  void* ret = NULL;
	  // loop over the code...
	  for(void* i=truth; i; i=cdr(i)) {

	    void* tmp = eval(car(i), env);;

	    if(is_error(tmp)) return tmp;

	    ret = tmp;
	  }

	  return ret;
	}
	else {
	  return NULL;
	}
      }
      break;

    case N_UNLESS:
      {
	void* pred = cdr(list);
	void* truth = cdr(cdr(list));
	
	if(!pred) {
	  return ERROR("ERROR: nothing to \?\?!");
	}

	if(!truth) {
	  return ERROR("Nothing to execute in ?? statement!\n");
	}
	
	void* predicate = eval(car(pred), env);
	
	if(!is_true(predicate)) {
	  void* ret = NULL;
	  // loop over the code...
	  for(void* i=truth; i; i=cdr(i)) {

	    void* tmp = eval(car(i), env);;

	    if(is_error(tmp)) return tmp;

	    ret = tmp;
	  }
	  
	  return ret;
	}
	else {
	  return NULL;
	}
      }
      break;
      
    case N_COND:
      {
	if(!cdr(list) || !car(cdr(list))) {
	  return ERROR("ERROR: COND requires one argument!");
	}

	for(void* i=car(cdr(list)); i; i = cdr(i)) {
	  
	  cc pair = car(i);
	  
	  if(is_true(eval(car(pair), env))) {
	    return eval(car(cdr(pair)), env);
	  }
	}
	return NULL;
      }
      break;
    
    case N_TYPE:
      if(!cdr(list)) {
	return ERROR("TYPE requires ONE argument!\n");
      }

      if(cdr(cdr(list))) {
	return ERROR("TYPE requires only ONE argument!\n");
      }
      
      return return_type(eval(car(cdr(list)), env));
      break;
      
    case N_NOT:
      if(!cdr(list)) {
	return ERROR("NOT requires ONE argument!\n");
      }

      if(cdr(cdr(list))) {
	return ERROR("NOT requires only ONE argument!\n");
      }

      void* result = eval(car(cdr(list)), env);
      if(is_true(result)) return NULL;
      else return create_true_type();
      break;
      
    case N_AND:

      {
	void* result = create_true_type();
	for(void* i=cdr(list); i; i = cdr(i)) {
	  result = eval(car(i), env);

	  if(!is_true(result)) return NULL;
	}

	return result;
      }
      break;

    case N_OR:
      
      {
	void* result = create_true_type();
	for(void* i=cdr(list); i; i = cdr(i)) {
	  result = eval(car(i), env);

	  if(is_true(result)) return result;
	}

	return NULL;
      }
      break;
      
    case N_APPEND:
      {
	if(!cdr(list) || !cdr(cdr(list))) {
	  return ERROR("ERROR: APPEND requires TWO arguments!\n");
	}

	cc tmp = car(cdr(list)); 
	
	cc l = car(cdr(cdr(list))); 

	return append(tmp, l);
      }
      break;
      
    case N_ASSOC:

      {
	if(!cdr(list) || !cdr(cdr(list))) {
	  return ERROR("ERROR: ASSOC requires at least TWO arguments!\n");
	}
	
	void* item = eval(car(cdr(list)), env);
	void* db = eval(car(cdr(cdr(list))), env);

	return assoc(item, db);
      }
      break;
      
    case N_EVAL:
      if(!cdr(list) || !car(cdr(list))) {
	return ERROR("EVAL requires ONE argument!\n");
      }

      return eval(eval(car(cdr(list)), env), env);
      break;
      
    case N_EQL:
      {
	if(!cdr(list)) {
	  return ERROR("ERROR: EQL requires 2 arguements!");
	}
	if(!cdr(cdr(list))) {
	  return ERROR("ERROR: EQL requires 2 arguements!");
	}

	void* tmp = cdr(list);
	void* a = eval(car(tmp), env);
	void* b = eval(car(cdr(tmp)), env);
	return equal(a, b);
      }
      break;

      case N_NEQL:
      {
	if(!cdr(list)) {
	  return ERROR("ERROR: EQL requires 2 arguements!");
	}
	if(!cdr(cdr(list))) {
	  return ERROR("ERROR: EQL requires 2 arguements!");
	}

	void* tmp = cdr(list);
	void* a = eval(car(tmp), env);
	void* b = eval(car(cdr(tmp)), env);

	if(compare(a, b) != 0) {
	  return create_true_type();
	}
	else {
	  return NULL;
	}
      }
      break;
      
    case N_LT:
      {
      	if(!cdr(list)) {
	  return ERROR("ERROR: < requires 2 arguements!");
	}
	if(!cdr(cdr(list))) {
	  return ERROR("ERROR: < requires 2 arguements!");
	}
	
	void* tmp = cdr(list);
	void* a = eval(car(tmp), env);
	void* b = eval(car(cdr(tmp)), env);
	
	if(compare(a, b) < 0) {
	  return create_true_type();
	}
	else {
	  return NULL;
	}
      }
      break;
      
    case N_GT:
      {
	if(!cdr(list)) {
	  return ERROR("ERROR: > requires 2 arguements!");
	}
	if(!cdr(cdr(list))) {
	  return ERROR("ERROR: > requires 2 arguements!");
	}
	
	void* tmp = cdr(list);
	void* a = eval(car(tmp), env);
	void* b = eval(car(cdr(tmp)), env);
	
	if(compare(a, b) > 0) {
	  return create_true_type();
	}
	else {
	  return NULL;
	}
      }
      break;
      
    case N_LTE:
      {
	if(!cdr(list)) {
	  return ERROR("ERROR: <= requires 2 arguements!");
	}
	if(!cdr(cdr(list))) {
	  return ERROR("ERROR: <= requires 2 arguements!");
	}
	
	void* tmp = cdr(list);
	void* a = eval(car(tmp), env);
	void* b = eval(car(cdr(tmp)), env);
	
	if(compare(a, b) <= 0) {
	  return create_true_type();
	}
	else {
	  return NULL;
	}
      }
      break;
      
    case N_GTE:
      {
	if(!cdr(list)) {
	  return ERROR("ERROR: >= requires 2 arguements!");
	}
	if(!cdr(cdr(list))) {
	  return ERROR("ERROR: >= requires 2 arguements!");
	}
	
	void* tmp = cdr(list);
	void* a = eval(car(tmp), env);
	void* b = eval(car(cdr(tmp)), env);
	
	if(compare(a, b) >= 0) {
	  return create_true_type();
	}
	else {
	  return NULL;
	}
      }
      break;
      
    case N_TO_STRING:
      {
	if(!cdr(list)) {
	  return ERROR("TO-STRING requires ONE argument!\n");
	}

	void* p = eval(car(cdr(list)), env);
	return to_string_type(p, 10);
      }
      break;
      
    case N_PRINT:
      {
	void* ret = eval(car(cdr(list)), env);
	print(stdout, ret, 10);
	fputc('\n', stdout);
	return ret;
      }
      break;
      
    case N_SET:
      {
	if(!cdr(list) || !cdr(cdr(list))) {
	  return ERROR("ERROR: SET requires TWO arguments!\n");
	}

	void* name = car(cdr(list));
	void* value = car(cdr(cdr(list)));
	
	for(void* i=car(env); i; i=cdr(i)) {

	  switch(get_type(car(i))) {

	  case TYPE_RB_TREE:
	    {
	      void* pair = mapget_pair(car(i), name, env);
	      if(is_error(pair)) return pair;

	      value = eval(value, env);
	      if(is_error(value)) return value;

	      if(pair) {
		cdr(pair) = value;
	      }
	      else {
		mapset(car(i), name, value, env);
	      }
	      return value;
	    }
	    break;

	  case TYPE_CONS:
	    {
	      void* found = assoc(name, car(i));

	      value = eval(value, env);
	    
	      if(is_error(value)) return value;
	  
	      cdr(found) = value;
	      return value;
	    }
	    break;

	  default:
	    ERROR("Set found an issue with the environment!!!\n");
	    break;
	  }
	  
	}
      }
	
      break;

    case N_WHILE:
      {
	void* pred = car(cdr(list));
	void* code = cdr(cdr(list));
	
	if(!pred) {
	  return ERROR("ERROR: nothing to WHILE!\n");
	}

	if(!code) {
	  return ERROR("Nothing to execute in WHILE statement!\n");
	}

	void* ret = NULL;
	while(is_true(eval(pred, env))) {
	  for(void* i=code; i; i = cdr(i)) {
	    ret = eval(car(i), env);
	  }
	}
	return ret;
      }	
      break;

    case N_ADD:
      {
	if(!car(cdr(list)) || !cdr(cdr(list)) || !car(cdr(cdr(list)))) {
	  return ERROR("ADD requires at least two arguments!");
	}
	void* a = eval(car(cdr(list)), env);
	
	if(is_error(a)) return a;
	
	for(cc i = cdr(cdr(list)); i; i=cdr(i)) {

	  void* b = eval(car(i), env);
	  if(is_error(b)) return b;
	  
	  switch(get_type(a)) {

	  case TYPE_INT:
	    
	    switch(get_type(b)) {

	    case TYPE_INT:
	      {

		int_type* result = create_int_type(0);
		mpz_add(result->num, to_int(a)->num, to_int(b)->num);
		a = result;
	      }
	      break;

	    case TYPE_FLOAT:
	      {
		float_type* af = create_float_type(); 
		mpf_set_z(af->num, to_int(a)->num);
		mpf_add(af->num, af->num, to_float(b)->num);
		a = af;
	      }
	      break;

	    case TYPE_RATIONAL:
	      {
		rational_type* ar = create_rational_type(); 
		mpq_set_z(ar->num, to_int(a)->num);
		mpq_add(ar->num, ar->num, to_rational(b)->num);
		
		a = ar;
	      }
	
	      break;

	    default:

	      return ERROR("Only integers, floats and rationals can be added!");
		      
	      break;
	    }

	    break;

	  case TYPE_FLOAT:
	        
	    switch(get_type(b)) {

	    case TYPE_INT:
	      {
		float_type* result = create_float_type();
		mpf_set_z(result->num, to_int(b)->num);
		mpf_add(result->num, to_float(a)->num, result->num);
		a = result;
	      }
	      break;

	    case TYPE_FLOAT:
	      {
		float_type* af = create_float_type(); 
		mpf_add(af->num, to_float(a)->num, to_float(b)->num);
		a = af;
	      }
	      break;

	    case TYPE_RATIONAL:
	      {
		float_type* af = create_float_type();
		mpf_set_q(af->num, to_rational(b)->num);
		mpf_add(af->num, to_float(a)->num, af->num);
		a = af;
	      }

	      break;

	    default:

	      return ERROR("Only integers, floats and rationals can be added!");
		      
	      break;
	    }

	    break;

	  case TYPE_RATIONAL:
	    switch(get_type(b)) {

	    case TYPE_INT:
	      {
		rational_type* result = create_rational_type();
		mpq_set_z(result->num, to_int(b)->num);
		mpq_add(result->num, to_rational(a)->num, result->num);
		a = result;
	      }
	      break;

	    case TYPE_FLOAT:
	      {
		float_type* af = create_float_type();
		mpf_set_q(af->num, to_rational(a)->num);
		mpf_add(af->num, af->num, to_float(b)->num);
		a = af;
	      }
	      break;

	    case TYPE_RATIONAL:
	      {
		rational_type* ar = create_rational_type();
		mpq_add(ar->num, to_rational(a)->num, to_rational(b)->num);
		a = ar;
	      }

	      break;

	    default:

	      return ERROR("Only integers, floats and rationals can be added!");
		      
	      break;
	    }

	    break;

	  default:

	    return ERROR("Only integers, floats and rationals can be added!");
		      
	    break;
	  }
	  
	}

	if(is_rational(a)) {
	  mpq_canonicalize(to_rational(a)->num);

	  if (mpz_cmp_ui(mpq_denref(to_rational(a)->num), 1) == 0) {

	    int_type* newint = create_int_type(0);
	    mpz_set(newint->num, mpq_numref(to_rational(a)->num));
	    a = newint;
	  }
	}
	
        return a;
      }	
      break;

    case N_SUB:
      {
	if(!car(cdr(list))) {
	  return ERROR("SUB requires at least one argument!");
	}

	void* a = eval(car(cdr(list)), env);
	if(is_error(a)) return a;

	if(!cdr(cdr(list))) {
	  	  
	  switch(get_type(a)) {
	  case TYPE_INT:
	    {
	      int_type* ret = create_int_type(0);
	      mpz_neg(ret->num, to_int(a)->num);
	      return ret;
	    }
	    break;
	  case TYPE_FLOAT:
	    {
	      float_type* ret = create_float_type();
	      mpf_neg(ret->num, to_float(a)->num);
	      return ret;
	    }
	    break;
	    
	  case TYPE_RATIONAL:
	    {
	      rational_type* ret = create_rational_type();
	      mpq_neg(ret->num, to_rational(a)->num);
	      return ret;
	    }
	    break;
	    	    
	  default:
	    return ERROR("Only integers, floats and rationals can be added!");	      
	    break;
	  }
	}
	
	for(cc i = cdr(cdr(list)); i; i=cdr(i)) {

	  void* b = eval(car(i), env);
	  if(is_error(b)) return b;
	  
	  switch(get_type(a)) {

	  case TYPE_INT:
	    
	    switch(get_type(b)) {

	    case TYPE_INT:
	      {
		int_type* result = create_int_type(0);
		mpz_sub(result->num, to_int(a)->num, to_int(b)->num);
		a = result;
	      }
	      break;

	    case TYPE_FLOAT:
	      {
		float_type* af = create_float_type(); 
		mpf_set_z(af->num, to_int(a)->num);
		mpf_sub(af->num, af->num, to_float(b)->num);
		a = af;
	      }
	      break;

	    case TYPE_RATIONAL:
	      {
		rational_type* ar = create_rational_type(); 
		mpq_set_z(ar->num, to_int(a)->num);
		mpq_sub(ar->num, ar->num, to_rational(b)->num);
		a = ar;
	      }
	
	      break;

	    default:

	      return ERROR("Only integers, floats and rationals can be added!");	      
	      break;
	    }

	    break;

	  case TYPE_FLOAT:

	    switch(get_type(b)) {

	    case TYPE_INT:
	      {
		float_type* result = create_float_type();
		mpf_set_z(result->num, to_int(b)->num);
		mpf_sub(result->num, to_float(a)->num, result->num);
		a = result;
	      }
	      break;

	    case TYPE_FLOAT:
	      {
		float_type* af = create_float_type(); 
		mpf_sub(af->num, to_float(a)->num, to_float(b)->num);
		a = af;
	      }
	      break;

	    case TYPE_RATIONAL:
	      {
		float_type* ar = create_float_type(); 
	     	mpf_set_q(ar->num, to_rational(b)->num);
	        mpf_sub(ar->num, to_float(a)->num, ar->num); 
	        a = ar; 
	      }
	
	      break;

	    default:

	      return ERROR("Only integers, floats and rationals can be added!");
		      
	      break;
	    }

	    break;

	  case TYPE_RATIONAL:
	    switch(get_type(b)) {

	    case TYPE_INT:
	      {
		rational_type* bq = create_rational_type();
		mpq_set_z(bq->num, to_int(b)->num);
		mpq_sub(bq->num, to_rational(a)->num, bq->num);
		a = bq;
	      }
	      break;

	    case TYPE_FLOAT:
	      {
		float_type* af = create_float_type();
		mpf_set_q(af->num, to_rational(a)->num);
		mpf_sub(af->num, af->num, to_float(b)->num);
		a = af;
	      }
	      break;

	    case TYPE_RATIONAL:
	      {
		rational_type* ar = create_rational_type();
		mpq_sub(ar->num, to_rational(a)->num, to_rational(b)->num);
		a = ar;
	      }
	      break;

	    default:

	      return ERROR("Only integers, floats and rationals can be added!");

	      break;
	    }

	    break;

	  default:

	    return ERROR("Only integers, floats and rationals can be added!");

	    break;
	  }

	}

	if(is_rational(a)) {
	  mpq_canonicalize(to_rational(a)->num);

	  if (mpz_cmp_ui(mpq_denref(to_rational(a)->num), 1) == 0) {

	    int_type* newint = create_int_type(0);
	    mpz_set(newint->num, mpq_numref(to_rational(a)->num));
	    a = newint;
	  }
	}
	
        return a;
      }	
      break;

    case N_MULT:
      {
	if(!car(cdr(list)) || !cdr(cdr(list)) || !car(cdr(cdr(list)))) {
	  return ERROR("ADD requires at least two arguments!");
	}

	void* a = eval(car(cdr(list)), env);
	if(is_error(a)) return a;
		
	for(cc i = cdr(cdr(list)); i; i=cdr(i)) {

	  void* b = eval(car(i), env);
	  if(is_error(b)) return b;
	  
	  switch(get_type(a)) {

	  case TYPE_INT:
	    
	    switch(get_type(b)) {

	    case TYPE_INT:
	      {
		int_type* result = create_int_type(0);
		mpz_mul(result->num, to_int(a)->num, to_int(b)->num);
		a = result;
	      }
	      break;

	    case TYPE_FLOAT:
	      {
		float_type* af = create_float_type(); 
		mpf_set_z(af->num, to_int(a)->num);
		mpf_mul(af->num, af->num, to_float(b)->num);
		a = af;
	      }
	      break;

	    case TYPE_RATIONAL:
	      {
		rational_type* ar = create_rational_type(); 
		mpq_set_z(ar->num, to_int(a)->num);
		mpq_mul(ar->num, ar->num, to_rational(b)->num);
		a = ar;
	      }
	
	      break;

	    default:

	      return ERROR("Only integers, floats and rationals can be added!");
		      
	      break;
	    }

	    break;

	  case TYPE_FLOAT:

	    switch(get_type(b)) {

	    case TYPE_INT:
	      {
		float_type* result = create_float_type();
		mpf_set_z(result->num, to_int(b)->num);
		mpf_mul(result->num, to_float(a)->num, result->num);
		a = result;
	      }
	      break;

	    case TYPE_FLOAT:
	      {
		float_type* af = create_float_type(); 
		mpf_mul(af->num, to_float(a)->num, to_float(b)->num);
		a = af;
	      }
	      break;

	    case TYPE_RATIONAL:
	      {
		float_type* ar = create_float_type(); 
	     	mpf_set_q(ar->num, to_rational(b)->num);
	        mpf_mul(ar->num, to_float(a)->num, ar->num); 
	        a = ar; 
	      }
	
	      break;

	    default:

	      return ERROR("Only integers, floats and rationals can be added!");
		      
	      break;
	    }

	    break;

	  case TYPE_RATIONAL:
	    switch(get_type(b)) {

	    case TYPE_INT:
	      {
		rational_type* bq = create_rational_type();
		mpq_set_z(bq->num, to_int(b)->num);
		mpq_mul(bq->num, to_rational(a)->num, bq->num);
		a = bq;
	      }
	      break;

	    case TYPE_FLOAT:
	      {
		float_type* af = create_float_type();
		mpf_set_q(af->num, to_rational(a)->num);
		mpf_mul(af->num, af->num, to_float(b)->num);
		a = af;
	      }
	      break;

	    case TYPE_RATIONAL:
	      {
		rational_type* ar = create_rational_type();
		mpq_mul(ar->num, to_rational(a)->num, to_rational(b)->num);
		a = ar;
	      }
	      break;

	    default:

	      return ERROR("Only integers, floats and rationals can be added!");

	      break;
	    }

	    break;

	  default:

	    return ERROR("Only integers, floats and rationals can be added!");

	    break;
	  }

	}

	if(is_rational(a)) {
	  mpq_canonicalize(to_rational(a)->num);

	  if (mpz_cmp_ui(mpq_denref(to_rational(a)->num), 1) == 0) {

	    int_type* newint = create_int_type(0);
	    mpz_set(newint->num, mpq_numref(to_rational(a)->num));
	    a = newint;
	  }
	}
	
        return a;
      }	
      break;

    case N_DIV:
      {
	if(!car(cdr(list))) {
	  return ERROR("DIV requires at least one argument!");
	}

	void* a = eval(car(cdr(list)), env);
	if(is_error(a)) return a;	
	if(!cdr(cdr(list))) {
	  
	  switch(get_type(a)) {
	  case TYPE_INT:
	    {
	      if(mpz_sgn(to_int(a)->num) == 0) {
		return ERROR("DIVIDE BY ZERO!!!");
	      }
	      
	      rational_type* ret = create_rational_type();
	      mpq_set_z(ret->num, to_int(a)->num);
	      mpq_inv(ret->num, ret->num);
	      mpq_canonicalize(ret->num);
	      return ret;
	    }
	    break;
	  case TYPE_FLOAT:
	    {
	      if (mpf_sgn(to_float(a)->num) == 0) {
		return ERROR("DIVIDE BY ZERO!!!");
	      }

	      float_type* ret = create_float_type();
	      mpf_ui_div(ret->num, 1, to_float(a)->num);
	      return ret;
	    }
	    break;
	    
	  case TYPE_RATIONAL:
	    {
	      if (mpq_sgn(to_rational(a)->num) == 0) {
		return ERROR("DIVIDE BY ZERO!!!");
	      }
	      
	      rational_type* ret = create_rational_type();
	      mpq_inv(ret->num, to_rational(a)->num);
	      return ret;
	    }
	    break;
	    
	  default:
	    return ERROR("Only integers, floats and rationals can be added!");	      
	    break;
	  }
	}
	
	
	for(cc i = cdr(cdr(list)); i; i=cdr(i)) {

	  void* b = eval(car(i), env);
	  if(is_error(b)) return a;
	  
	  switch(get_type(a)) {

	  case TYPE_INT:
	    
	    switch(get_type(b)) {

	    case TYPE_INT:
	      {
				
		if(mpz_sgn(to_int(b)->num) == 0) {
		  return ERROR("DIVIDE BY ZERO!!!");
		}
		
		rational_type* aq = create_rational_type();
		rational_type* bq = create_rational_type();
		
		mpq_set_z(aq->num, to_int(a)->num);
		mpq_set_z(bq->num, to_int(b)->num);
		mpq_div(aq->num, aq->num, bq->num);
		a = aq;
	      }
	      break;

	    case TYPE_FLOAT:
	      {
		if (mpf_sgn(to_float(b)->num) == 0) {
		  return ERROR("DIVIDE BY ZERO!!!");
		}
				
		float_type* af = create_float_type();
		mpf_set_z(af->num, to_int(a)->num);
		mpf_div(af->num, af->num, to_float(b)->num);
		a = af;
	      }
	      break;

	    case TYPE_RATIONAL:
	      {
		if (mpq_sgn(to_rational(b)->num) == 0) {
		  return ERROR("DIVIDE BY ZERO!!!");
		}
				
		rational_type* ar = create_rational_type(); 
		mpq_set_z(ar->num, to_int(a)->num);
		mpq_div(ar->num, ar->num, to_rational(b)->num);
		a = ar;
	      }
	      break;

	    default:

	      return ERROR("Only integers, floats and rationals can be added!");
		      
	      break;
	    }

	    break;

	  case TYPE_FLOAT:

	    switch(get_type(b)) {

	    case TYPE_INT:
	      {
		if(mpz_sgn(to_int(b)->num) == 0) {
		  return ERROR("DIVIDE BY ZERO!!!");
		}
				
		float_type* result = create_float_type();
		mpf_set_z(result->num, to_int(b)->num);
		mpf_div(result->num, to_float(a)->num, result->num);
		a = result;
	      }
	      break;

	    case TYPE_FLOAT:
	      {
		if (mpf_sgn(to_float(b)->num) == 0) {
		  return ERROR("DIVIDE BY ZERO!!!");
		}
		
		float_type* af = create_float_type(); 
		mpf_div(af->num, to_float(a)->num, to_float(b)->num);
		a = af;
	      }
	      break;

	    case TYPE_RATIONAL:
	      {
		if (mpq_sgn(to_rational(b)->num) == 0) {
		  return ERROR("DIVIDE BY ZERO!!!");
		}
		
		float_type* ar = create_float_type(); 
	     	mpf_set_q(ar->num, to_rational(b)->num);
	        mpf_div(ar->num, to_float(a)->num, ar->num); 
	        a = ar; 
	      }
	
	      break;

	    default:

	      return ERROR("Only integers, floats and rationals can be added!");
		      
	      break;
	    }

	    break;

	  case TYPE_RATIONAL:
	    switch(get_type(b)) {
	      
	    case TYPE_INT:
	      {
		if(mpz_sgn(to_int(b)->num) == 0) {
		  return ERROR("DIVIDE BY ZERO!!!");
		}
				
		rational_type* result = create_rational_type();
		mpq_set_z(result->num, to_int(b)->num);
		mpq_div(result->num, to_rational(a)->num, result->num);
		a = result;
	      }
	      break;

	    case TYPE_FLOAT:
	      {
		if (mpf_sgn(to_float(b)->num) == 0) {
		  return ERROR("DIVIDE BY ZERO!!!");
		}
		
		float_type* af = create_float_type();
		mpf_set_q(af->num, to_rational(a)->num);
		mpf_div(af->num, to_float(a)->num, to_float(b)->num);
		a = af;
	      }
	      break;

	    case TYPE_RATIONAL:
	      {
		if (mpq_sgn(to_rational(b)->num) == 0) {
		  return ERROR("DIVIDE BY ZERO!!!");
		}
		
		rational_type* ret = create_rational_type(); 
		mpq_div(ret->num, to_rational(a)->num, to_rational(b)->num); 
	        a = ret; 
	      }
	
	      break;

	    default:

	      return ERROR("Only integers, floats and rationals can be added!");
		      
	      break;
	    }

	    break;

	  default:

	    return ERROR("Only integers, floats and rationals can be added!");
		      
	    break;
	  }
	  
	}
	
        return a;
      }	
      
      break;


      
    case N_BREAK:
      {

	void* pair = cassoc("*BREAK*", cdr(env));
	if(!pair) {
	  return ERROR("BREAK outside of LOOP!!!");
	}
	
	if(cdr(list) && car(cdr(list))) {
	  cdr(pair) = eval(car(cdr(list)), env);
	}
	else {
	  cdr(pair) = create_true_type();
	}
	
	return cdr(pair);
      }
      break;
      
    case N_LOOP:
      {
	void* start = cdr(list);
	if(!start || !car(start)) {
	  return ERROR("LOOP requires at least one argument!");
	}

	// Fresh env cell, never mutates the caller's env -- same pattern as
	// the TYPE_LAMBDA/apply_callable fix: nothing here is reachable from
	// env, so there is nothing to restore, and a future non-local exit
	// (a restart) can safely unwind through this frame without leaking
	// a stale *BREAK* binding onto shared state.
	void* newenv = cons(car(env), cons(cons(create_symbol("*BREAK*"), NULL), cdr(env)));

	void* i = start;
	void* ret = cdr(cassoc("*BREAK*", cdr(newenv)));
	while(!ret) {
	  eval(car(i), newenv);
	  ret = cdr(cassoc("*BREAK*", cdr(newenv)));

	  if(cdr(i)) i = cdr(i);
	  else       i = start;
	}

	return ret;
      }
      break;

    // A minimal restart system -- NOT a full Common Lisp condition system
    // (no condition classes, no signal/handler-bind, no search-by-type).
    // Exactly one dynamic-extent recovery point, established lexically,
    // found dynamically by name, invoked to transfer control back to its
    // establishment point with a value. ERROR-as-a-returned-value is
    // untouched: this is for structured recovery control flow, not for
    // representing failures, which continue to work via ERROR()/is_error()
    // exactly as before.
    case N_WITH_RESTART:
      {
	void* rest = cdr(list);
	if(!rest || !car(rest)) {
	  return ERROR("WITH-RESTART requires a name!");
	}
	void* name = car(rest);
	if(!is_type(name, TYPE_SYMBOL)) {
	  return ERROR("WITH-RESTART: name must be a symbol!");
	}

	rest = cdr(rest);
	if(!rest || !car(rest)) {
	  return ERROR("WITH-RESTART requires a recovery lambda!");
	}
	void* recovery = eval(car(rest), env);
	if(is_error(recovery)) return recovery;

	void* body = cdr(rest);

	restart_frame* frame = GC_malloc(sizeof(restart_frame));
	frame->value = NULL;

	if(setjmp(frame->buf)) {
	  // INVOKE-RESTART jumped back here.
	  void* args = cons(frame->value, NULL);
	  return apply_callable(recovery, args, ARGS_VALUES, env, env);
	}

	// Fresh env cell, same pattern as N_LOOP's *BREAK* frame -- never
	// mutates the caller's env, so there is nothing to leak or restore
	// if this dynamic extent ends via a normal return. TYPE_RESTART
	// (rather than the generic TYPE_POINTER) tags this binding so
	// AVAILABLERESTARTS can walk cdr(env) and pick out restart entries
	// unambiguously, distinct from *BREAK* and any other dynamic binding
	// that might share this same list.
	void* newenv = cons(car(env),
			     cons(cons(to_string(name), create_quotetype(TYPE_RESTART, frame)),
				  cdr(env)));

	void* ret = NULL;
	for(void* i=body; i; i=cdr(i)) {

	  void* tmp = eval(car(i), newenv);

	  if(is_error(tmp)) return tmp;

	  ret = tmp;
	}

	return ret;
      }
      break;

    case N_INVOKE_RESTART:
      {
	void* rest = cdr(list);
	if(!rest || !car(rest)) {
	  return ERROR("INVOKE-RESTART requires a name!");
	}
	void* name = car(rest);
	if(!is_type(name, TYPE_SYMBOL)) {
	  return ERROR("INVOKE-RESTART: name must be a symbol!");
	}

	void* pair = cassoc(to_string(name)->str, cdr(env));
	if(!pair || !is_type(cdr(pair), TYPE_RESTART)) {
	  return ERROR("INVOKE-RESTART: no such restart!");
	}

	restart_frame* frame = car(cdr(pair));

	void* valueForm = cdr(rest);
	if(valueForm && car(valueForm)) {
	  frame->value = eval(car(valueForm), env);
	  if(is_error(frame->value)) return frame->value;
	}
	else {
	  frame->value = NULL;
	}

	longjmp(frame->buf, 1);
	// never reached
	return NULL;
      }
      break;

    case N_AVAILABLERESTARTS:
      {
	// (AVAILABLERESTARTS) -- names of every currently-active restart,
	// innermost first, matching cassoc's own search order. Walks
	// cdr(env) the same way WITH-RESTART/INVOKE-RESTART do, filtering
	// on the TYPE_RESTART tag so *BREAK* and any other dynamic binding
	// sharing this list is skipped.
	void* ret = NULL;
	void* last = NULL;

	for(void* i=cdr(env); i; i=cdr(i)) {
	  void* pair = car(i);
	  if(!pair || !is_cons(pair) || !is_type(cdr(pair), TYPE_RESTART)) continue;

	  void* name = car(pair);
	  if(!ret) { ret = cons(name, NULL); last = ret; }
	  else { cdr(last) = cons(name, NULL); last = cdr(last); }
	}

	return ret;
      }
      break;

    case N_APPLY:
      {
	// (APPLY callee valueList) -- valueList's elements are bound as-is,
	// never re-evaluated, since they're already values.
	void* a1 = cdr(list);
	if(!a1 || !car(a1)) return ERROR("APPLY requires 2 arguments!");
	void* a2 = cdr(a1);
	if(!a2 || !car(a2)) return ERROR("APPLY requires 2 arguments!");

	void* callee = eval(car(a1), env);
	if(is_error(callee)) return callee;

	void* values = eval(car(a2), env);
	if(is_error(values)) return values;
	if(values && !is_cons(values)) return ERROR("APPLY requires a list!");

	return apply_callable(callee, values, ARGS_VALUES, env, env);
      }
      break;

    case N_MOD:
      {
	// (MOD a b) -- integer modulo, sign follows the divisor (GMP mpz_mod).
	void* a1 = cdr(list);
	if(!a1 || !car(a1)) return ERROR("MOD requires 2 arguments!");
	void* a2 = cdr(a1);
	if(!a2 || !car(a2)) return ERROR("MOD requires 2 arguments!");

	void* a = eval(car(a1), env);
	if(is_error(a)) return a;
	void* b = eval(car(a2), env);
	if(is_error(b)) return b;

	if(!is_int(a) || !is_int(b)) return ERROR("MOD requires two integers!");
	if(mpz_sgn(to_int(b)->num) == 0) return ERROR("DIVIDE BY ZERO!!!");

	int_type* ret = create_int_type(0);
	mpz_mod(ret->num, to_int(a)->num, to_int(b)->num);
	return ret;
      }
      break;

    case N_QUOTIENT:
      {
	// (QUOTIENT a b) -- truncating integer division.
	void* a1 = cdr(list);
	if(!a1 || !car(a1)) return ERROR("QUOTIENT requires 2 arguments!");
	void* a2 = cdr(a1);
	if(!a2 || !car(a2)) return ERROR("QUOTIENT requires 2 arguments!");

	void* a = eval(car(a1), env);
	if(is_error(a)) return a;
	void* b = eval(car(a2), env);
	if(is_error(b)) return b;

	if(!is_int(a) || !is_int(b)) return ERROR("QUOTIENT requires two integers!");
	if(mpz_sgn(to_int(b)->num) == 0) return ERROR("DIVIDE BY ZERO!!!");

	int_type* ret = create_int_type(0);
	mpz_tdiv_q(ret->num, to_int(a)->num, to_int(b)->num);
	return ret;
      }
      break;

    case N_REMAINDER:
      {
	// (REMAINDER a b) -- truncating-division remainder, sign follows
	// the dividend (distinct from MOD, whose sign follows the divisor).
	void* a1 = cdr(list);
	if(!a1 || !car(a1)) return ERROR("REMAINDER requires 2 arguments!");
	void* a2 = cdr(a1);
	if(!a2 || !car(a2)) return ERROR("REMAINDER requires 2 arguments!");

	void* a = eval(car(a1), env);
	if(is_error(a)) return a;
	void* b = eval(car(a2), env);
	if(is_error(b)) return b;

	if(!is_int(a) || !is_int(b)) return ERROR("REMAINDER requires two integers!");
	if(mpz_sgn(to_int(b)->num) == 0) return ERROR("DIVIDE BY ZERO!!!");

	int_type* ret = create_int_type(0);
	mpz_tdiv_r(ret->num, to_int(a)->num, to_int(b)->num);
	return ret;
      }
      break;

    case N_FLOOR:
      {
	void* a1 = cdr(list);
	if(!a1 || !car(a1)) return ERROR("FLOOR requires 1 argument!");

	void* a = eval(car(a1), env);
	if(is_error(a)) return a;

	if(is_int(a)) return a;

	if(is_float(a)) {
	  float_type* ret = create_float_type();
	  mpf_floor(ret->num, to_float(a)->num);
	  return ret;
	}

	if(is_rational(a)) {
	  int_type* ret = create_int_type(0);
	  mpz_fdiv_q(ret->num, mpq_numref(to_rational(a)->num), mpq_denref(to_rational(a)->num));
	  return ret;
	}

	return ERROR("FLOOR requires a number!");
      }
      break;

    case N_CEILING:
      {
	void* a1 = cdr(list);
	if(!a1 || !car(a1)) return ERROR("CEILING requires 1 argument!");

	void* a = eval(car(a1), env);
	if(is_error(a)) return a;

	if(is_int(a)) return a;

	if(is_float(a)) {
	  float_type* ret = create_float_type();
	  mpf_ceil(ret->num, to_float(a)->num);
	  return ret;
	}

	if(is_rational(a)) {
	  int_type* ret = create_int_type(0);
	  mpz_cdiv_q(ret->num, mpq_numref(to_rational(a)->num), mpq_denref(to_rational(a)->num));
	  return ret;
	}

	return ERROR("CEILING requires a number!");
      }
      break;

    case N_TRUNCATE:
      {
	void* a1 = cdr(list);
	if(!a1 || !car(a1)) return ERROR("TRUNCATE requires 1 argument!");

	void* a = eval(car(a1), env);
	if(is_error(a)) return a;

	if(is_int(a)) return a;

	if(is_float(a)) {
	  float_type* ret = create_float_type();
	  mpf_trunc(ret->num, to_float(a)->num);
	  return ret;
	}

	if(is_rational(a)) {
	  int_type* ret = create_int_type(0);
	  mpz_tdiv_q(ret->num, mpq_numref(to_rational(a)->num), mpq_denref(to_rational(a)->num));
	  return ret;
	}

	return ERROR("TRUNCATE requires a number!");
      }
      break;

    case N_ROUND:
      {
	// Round-half-to-even. Ints pass through. Floats/rationals: take the
	// floor and the fractional remainder, decide based on the remainder
	// vs 1/2, breaking exact ties to the even neighbor.
	void* a1 = cdr(list);
	if(!a1 || !car(a1)) return ERROR("ROUND requires 1 argument!");

	void* a = eval(car(a1), env);
	if(is_error(a)) return a;

	if(is_int(a)) return a;

	if(is_float(a)) {
	  float_type* ret = create_float_type();
	  float_type* flo = create_float_type();
	  mpf_floor(flo->num, to_float(a)->num);

	  float_type* frac = create_float_type();
	  mpf_sub(frac->num, to_float(a)->num, flo->num);

	  float_type* half = create_float_type();
	  mpf_set_d(half->num, 0.5);

	  int cmp = mpf_cmp(frac->num, half->num);
	  if(cmp < 0) {
	    mpf_set(ret->num, flo->num);
	  }
	  else if(cmp > 0) {
	    mpf_add_ui(ret->num, flo->num, 1);
	  }
	  else {
	    // exact .5 -- round to even
	    mpz_t flooriz;
	    mpz_init(flooriz);
	    mpz_set_f(flooriz, flo->num);
	    if(mpz_even_p(flooriz)) {
	      mpf_set(ret->num, flo->num);
	    }
	    else {
	      mpf_add_ui(ret->num, flo->num, 1);
	    }
	    mpz_clear(flooriz);
	  }
	  return ret;
	}

	if(is_rational(a)) {
	  mpz_t floorz, num, den, rem2, den2;
	  mpz_init(floorz); mpz_init(num); mpz_init(den); mpz_init(rem2); mpz_init(den2);
	  mpz_set(num, mpq_numref(to_rational(a)->num));
	  mpz_set(den, mpq_denref(to_rational(a)->num));
	  mpz_fdiv_q(floorz, num, den);

	  // remainder = num - floor*den; compare 2*remainder to den
	  mpz_t rem, floorTimesDen;
	  mpz_init(rem); mpz_init(floorTimesDen);
	  mpz_mul(floorTimesDen, floorz, den);
	  mpz_sub(rem, num, floorTimesDen);
	  mpz_mul_ui(rem2, rem, 2);

	  int cmp = mpz_cmp(rem2, den);
	  int_type* ret = create_int_type(0);
	  if(cmp < 0) {
	    mpz_set(ret->num, floorz);
	  }
	  else if(cmp > 0) {
	    mpz_add_ui(ret->num, floorz, 1);
	  }
	  else {
	    // exact .5 -- round to even
	    if(mpz_even_p(floorz)) {
	      mpz_set(ret->num, floorz);
	    }
	    else {
	      mpz_add_ui(ret->num, floorz, 1);
	    }
	  }

	  mpz_clear(floorz); mpz_clear(num); mpz_clear(den);
	  mpz_clear(rem2); mpz_clear(den2); mpz_clear(rem); mpz_clear(floorTimesDen);
	  return ret;
	}

	return ERROR("ROUND requires a number!");
      }
      break;

    case N_ABS:
      {
	void* a1 = cdr(list);
	if(!a1 || !car(a1)) return ERROR("ABS requires 1 argument!");

	void* a = eval(car(a1), env);
	if(is_error(a)) return a;

	if(is_int(a)) {
	  int_type* ret = create_int_type(0);
	  mpz_abs(ret->num, to_int(a)->num);
	  return ret;
	}

	if(is_float(a)) {
	  float_type* ret = create_float_type();
	  mpf_abs(ret->num, to_float(a)->num);
	  return ret;
	}

	if(is_rational(a)) {
	  rational_type* ret = create_rational_type();
	  mpq_abs(ret->num, to_rational(a)->num);
	  return ret;
	}

	return ERROR("ABS requires a number!");
      }
      break;

    case N_SQRT:
      {
	// Floats via mpf_sqrt. Integers: mpz_sqrt truncates to the integer
	// square root -- always promote to float here, since GMP gives no
	// direct way to tell "was this exact" without a second check, and
	// this keeps the type contract simple (SQRT always returns a float,
	// except NULL is never returned). Rationals promote to float too.
	void* a1 = cdr(list);
	if(!a1 || !car(a1)) return ERROR("SQRT requires 1 argument!");

	void* a = eval(car(a1), env);
	if(is_error(a)) return a;

	if(is_int(a)) {
	  if(mpz_sgn(to_int(a)->num) < 0) return ERROR("SQRT of a negative number!");
	  float_type* ret = create_float_type();
	  mpf_t tmp;
	  mpf_init(tmp);
	  mpf_set_z(tmp, to_int(a)->num);
	  mpf_sqrt(ret->num, tmp);
	  mpf_clear(tmp);
	  return ret;
	}

	if(is_float(a)) {
	  if(mpf_sgn(to_float(a)->num) < 0) return ERROR("SQRT of a negative number!");
	  float_type* ret = create_float_type();
	  mpf_sqrt(ret->num, to_float(a)->num);
	  return ret;
	}

	if(is_rational(a)) {
	  if(mpq_sgn(to_rational(a)->num) < 0) return ERROR("SQRT of a negative number!");
	  float_type* ret = create_float_type();
	  mpf_t tmp;
	  mpf_init(tmp);
	  mpf_set_q(tmp, to_rational(a)->num);
	  mpf_sqrt(ret->num, tmp);
	  mpf_clear(tmp);
	  return ret;
	}

	return ERROR("SQRT requires a number!");
      }
      break;

    case N_EXPT:
      {
	// (EXPT base exp) -- exp must be a non-negative integer. Integer
	// base stays exact via mpz_pow_ui; float/rational base promotes
	// through <math.h> pow() for float, or repeated rational
	// multiplication for rational (kept exact).
	void* a1 = cdr(list);
	if(!a1 || !car(a1)) return ERROR("EXPT requires 2 arguments!");
	void* a2 = cdr(a1);
	if(!a2 || !car(a2)) return ERROR("EXPT requires 2 arguments!");

	void* base = eval(car(a1), env);
	if(is_error(base)) return base;
	void* exp = eval(car(a2), env);
	if(is_error(exp)) return exp;

	if(!is_int(exp) || mpz_sgn(to_int(exp)->num) < 0) {
	  return ERROR("EXPT requires a non-negative integer exponent!");
	}
	unsigned long e = mpz_get_ui(to_int(exp)->num);

	if(is_int(base)) {
	  int_type* ret = create_int_type(0);
	  mpz_pow_ui(ret->num, to_int(base)->num, e);
	  return ret;
	}

	if(is_rational(base)) {
	  rational_type* ret = create_rational_type();
	  mpz_pow_ui(mpq_numref(ret->num), mpq_numref(to_rational(base)->num), e);
	  mpz_pow_ui(mpq_denref(ret->num), mpq_denref(to_rational(base)->num), e);
	  mpq_canonicalize(ret->num);
	  return ret;
	}

	if(is_float(base)) {
	  float_type* ret = create_float_type();
	  double baseD = mpf_get_d(to_float(base)->num);
	  mpf_set_d(ret->num, pow(baseD, (double)e));
	  return ret;
	}

	return ERROR("EXPT requires a number base!");
      }
      break;

    case N_MIN:
      {
	void* a1 = cdr(list);
	if(!a1 || !car(a1)) return ERROR("MIN requires at least 1 argument!");

	void* best = eval(car(a1), env);
	if(is_error(best)) return best;

	for(void* i=cdr(a1); i; i=cdr(i)) {
	  void* v = eval(car(i), env);
	  if(is_error(v)) return v;
	  if(compare(v, best) < 0) best = v;
	}

	return best;
      }
      break;

    case N_MAX:
      {
	void* a1 = cdr(list);
	if(!a1 || !car(a1)) return ERROR("MAX requires at least 1 argument!");

	void* best = eval(car(a1), env);
	if(is_error(best)) return best;

	for(void* i=cdr(a1); i; i=cdr(i)) {
	  void* v = eval(car(i), env);
	  if(is_error(v)) return v;
	  if(compare(v, best) > 0) best = v;
	}

	return best;
      }
      break;

    case N_GCD:
      {
	void* a1 = cdr(list);
	if(!a1 || !car(a1)) return ERROR("GCD requires 2 arguments!");
	void* a2 = cdr(a1);
	if(!a2 || !car(a2)) return ERROR("GCD requires 2 arguments!");

	void* a = eval(car(a1), env);
	if(is_error(a)) return a;
	void* b = eval(car(a2), env);
	if(is_error(b)) return b;

	if(!is_int(a) || !is_int(b)) return ERROR("GCD requires two integers!");

	int_type* ret = create_int_type(0);
	mpz_gcd(ret->num, to_int(a)->num, to_int(b)->num);
	return ret;
      }
      break;

    case N_LCM:
      {
	void* a1 = cdr(list);
	if(!a1 || !car(a1)) return ERROR("LCM requires 2 arguments!");
	void* a2 = cdr(a1);
	if(!a2 || !car(a2)) return ERROR("LCM requires 2 arguments!");

	void* a = eval(car(a1), env);
	if(is_error(a)) return a;
	void* b = eval(car(a2), env);
	if(is_error(b)) return b;

	if(!is_int(a) || !is_int(b)) return ERROR("LCM requires two integers!");

	int_type* ret = create_int_type(0);
	mpz_lcm(ret->num, to_int(a)->num, to_int(b)->num);
	return ret;
      }
      break;

    case N_EXACTP:
      {
	void* a1 = cdr(list);
	if(!a1 || !car(a1)) return ERROR("EXACT? requires 1 argument!");

	void* a = eval(car(a1), env);
	if(is_error(a)) return a;

	return (is_int(a) || is_rational(a)) ? create_true_type() : NULL;
      }
      break;

    case N_INEXACTP:
      {
	void* a1 = cdr(list);
	if(!a1 || !car(a1)) return ERROR("INEXACT? requires 1 argument!");

	void* a = eval(car(a1), env);
	if(is_error(a)) return a;

	return is_float(a) ? create_true_type() : NULL;
      }
      break;

    case N_TYPEIS:
      {
	// (TYPE? x 'TAGNAME) -- TRUE iff x's type name (return_type_c_string)
	// equals TAGNAME's symbol name. The primitive underneath the
	// individual ?-named type predicates in prelude.safe.
	//
	// A few type-name spellings collide with reserved keywords (CONS,
	// TRUE, LAMBDA, MAC): quoting one of those words doesn't produce
	// the plain symbol "CONS" etc, it produces the keyword's own native
	// value (the lexer turns the bare word into that keyword's token
	// before quoting ever sees it). Those specific predicates (CONS?,
	// PROCEDURE?) get their own dedicated native forms instead of going
	// through TYPE?, so TYPE? itself only ever needs to handle a plain
	// symbol tag.
	void* a1 = cdr(list);
	if(!a1 || !car(a1)) return ERROR("TYPE? requires 2 arguments!");
	void* a2 = cdr(a1);
	if(!a2 || !car(a2)) return ERROR("TYPE? requires 2 arguments!");

	void* x = eval(car(a1), env);
	if(is_error(x)) return x;

	void* tag = eval(car(a2), env);
	if(is_error(tag)) return tag;
	if(!is_type(tag, TYPE_SYMBOL)) return ERROR("TYPE? requires a symbol tag!");

	return (strcmp(return_type_c_string(x), to_string(tag)->str) == 0)
	       ? create_true_type() : NULL;
      }
      break;

    case N_NULLP:
      {
	// (NULL? x) -- TRUE iff x's evaluated value is the NULL pointer.
	// Note: !car(a1) would wrongly reject a present-but-NULL argument as
	// "missing" -- check !a1 alone (is there a second list cell at all).
	void* a1 = cdr(list);
	if(!a1) return ERROR("NULL? requires 1 argument!");

	void* v = eval(car(a1), env);
	if(is_error(v)) return v;

	return (v == NULL) ? create_true_type() : NULL;
      }
      break;

    case N_CONSP:
      {
	// (CONS? x) -- TRUE iff x's evaluated value is a cons cell. Its own
	// dedicated native (not routed through TYPE?) since the word CONS
	// is itself a reserved keyword -- there's no way to quote it into
	// the plain symbol "CONS" as data to compare against.
	void* a1 = cdr(list);
	if(!a1 || !car(a1)) return ERROR("CONS? requires 1 argument!");

	void* v = eval(car(a1), env);
	if(is_error(v)) return v;

	return is_cons(v) ? create_true_type() : NULL;
      }
      break;

    case N_PROCEDUREP:
      {
	// (PROCEDURE? x) -- TRUE iff x is callable: a LAMBDA, a MAC, or a
	// bare native operator (TYPE_NATIVE_INT, e.g. + or CAR). Its own
	// dedicated native for the same reason as CONS? -- LAMBDA/MAC are
	// reserved keywords, not quotable as plain symbols.
	void* a1 = cdr(list);
	if(!a1 || !car(a1)) return ERROR("PROCEDURE? requires 1 argument!");

	void* v = eval(car(a1), env);
	if(is_error(v)) return v;

	ValueType t = get_type(v);
	return (t == TYPE_LAMBDA || t == TYPE_MACRO || t == TYPE_NATIVE_INT)
	       ? create_true_type() : NULL;
      }
      break;

    case N_LEN:
      {
	// (LEN x) -- one length primitive for both strings and lists.
	// list_length(NULL) returns -1 (its "not a proper list" sentinel,
	// since NULL isn't TYPE_CONS) -- special-case NULL to 0 rather than
	// passing that sentinel through as if it were a real length.
	// Note: check !a1 alone, not !car(a1) -- NULL is a legitimate value
	// to pass here (LEN NULL) should be 0, not "argument missing".
	void* a1 = cdr(list);
	if(!a1) return ERROR("LEN requires 1 argument!");

	void* x = eval(car(a1), env);
	if(is_error(x)) return x;

	if(x == NULL) return create_int_type(0);

	if(is_str(x)) return create_int_type(to_string(x)->size);

	if(is_cons(x)) {
	  int n = list_length(x);
	  if(n < 0) return ERROR("LEN: not a proper list!");
	  return create_int_type(n);
	}

	return ERROR("LEN requires a list or a string!");
      }
      break;

    case N_SUBSTR:
      {
	// (SUBSTR s start end) -- end exclusive, Scheme substring-style.
	void* a1 = cdr(list);
	if(!a1 || !car(a1)) return ERROR("SUBSTR requires 3 arguments!");
	void* a2 = cdr(a1);
	if(!a2 || !car(a2)) return ERROR("SUBSTR requires 3 arguments!");
	void* a3 = cdr(a2);
	if(!a3 || !car(a3)) return ERROR("SUBSTR requires 3 arguments!");

	void* s = eval(car(a1), env);
	if(is_error(s)) return s;
	if(!is_str(s)) return ERROR("SUBSTR requires a string!");

	void* startV = eval(car(a2), env);
	if(is_error(startV)) return startV;
	void* endV = eval(car(a3), env);
	if(is_error(endV)) return endV;
	if(!is_int(startV) || !is_int(endV)) return ERROR("SUBSTR requires integer bounds!");

	long start = mpz_get_si(to_int(startV)->num);
	long end = mpz_get_si(to_int(endV)->num);
	int size = to_string(s)->size;

	if(start < 0 || end < start || end > size) {
	  return ERROR("SUBSTR: index out of range!");
	}

	return create_string_type_and_copy(end - start, to_string(s)->str + start, TYPE_STRING);
      }
      break;

    case N_STRREF:
      {
	// (STRREF s i) -- single character, bounds-checked.
	void* a1 = cdr(list);
	if(!a1 || !car(a1)) return ERROR("STRREF requires 2 arguments!");
	void* a2 = cdr(a1);
	if(!a2 || !car(a2)) return ERROR("STRREF requires 2 arguments!");

	void* s = eval(car(a1), env);
	if(is_error(s)) return s;
	if(!is_str(s)) return ERROR("STRREF requires a string!");

	void* iV = eval(car(a2), env);
	if(is_error(iV)) return iV;
	if(!is_int(iV)) return ERROR("STRREF requires an integer index!");

	long i = mpz_get_si(to_int(iV)->num);
	if(i < 0 || i >= to_string(s)->size) return ERROR("STRREF: index out of range!");

	return create_char_type(to_string(s)->str[i]);
      }
      break;

    case N_STRUPPER:
      {
	void* a1 = cdr(list);
	if(!a1 || !car(a1)) return ERROR("STRUPPER requires 1 argument!");

	void* s = eval(car(a1), env);
	if(is_error(s)) return s;
	if(!is_str(s)) return ERROR("STRUPPER requires a string!");

	string_type* ret = create_string_type_and_copy(to_string(s)->size, to_string(s)->str, TYPE_STRING);
	for(int i = 0; i < ret->size; i++) ret->str[i] = toupper((unsigned char)ret->str[i]);
	return ret;
      }
      break;

    case N_STRLOWER:
      {
	void* a1 = cdr(list);
	if(!a1 || !car(a1)) return ERROR("STRLOWER requires 1 argument!");

	void* s = eval(car(a1), env);
	if(is_error(s)) return s;
	if(!is_str(s)) return ERROR("STRLOWER requires a string!");

	string_type* ret = create_string_type_and_copy(to_string(s)->size, to_string(s)->str, TYPE_STRING);
	for(int i = 0; i < ret->size; i++) ret->str[i] = tolower((unsigned char)ret->str[i]);
	return ret;
      }
      break;

    case N_STREQ:
      {
	void* a1 = cdr(list);
	if(!a1 || !car(a1)) return ERROR("STREQ requires 2 arguments!");
	void* a2 = cdr(a1);
	if(!a2 || !car(a2)) return ERROR("STREQ requires 2 arguments!");

	void* a = eval(car(a1), env);
	if(is_error(a)) return a;
	void* b = eval(car(a2), env);
	if(is_error(b)) return b;

	if(!is_str(a) || !is_str(b)) return ERROR("STREQ requires two strings!");

	return (string_compare(a, b) == 0) ? create_true_type() : NULL;
      }
      break;

    case N_VALUES:
      {
	// (VALUES a b c ...) -- zero or more. Wraps its evaluated arguments
	// in a TYPE_VALUES tag; eval()'s wrapper collapses that to the
	// first value everywhere except inside NTHVALUE/MULTIPLEVALUELIST,
	// which see the real, uncollapsed TYPE_VALUES by calling eval_raw
	// on this form directly instead of going through eval().
	cc ret = NULL;
	cc next = NULL;

	for(cc i=cdr(list); i; i=cdr(i)) {
	  void* tmp = eval(car(i), env);
	  if(is_error(tmp)) return tmp;

	  if(ret) {
	    cc c = cons(tmp, NULL);
	    next->cdr = c;
	    next = c;
	  }
	  else {
	    ret = cons(tmp, NULL);
	    next = ret;
	  }
	}

	return create_quotetype(TYPE_VALUES, ret);
      }
      break;

    case N_NTHVALUE:
      {
	// (NTHVALUE n expr) -- 0-indexed. expr is evaluated via eval_raw
	// (not eval) so a VALUES result arrives unwrapped rather than
	// already collapsed to its first element.
	void* a1 = cdr(list);
	if(!a1 || !car(a1)) return ERROR("NTHVALUE requires 2 arguments!");
	void* a2 = cdr(a1);
	if(!a2 || !car(a2)) return ERROR("NTHVALUE requires 2 arguments!");

	void* nV = eval(car(a1), env);
	if(is_error(nV)) return nV;
	if(!is_int(nV)) return ERROR("NTHVALUE requires an integer index!");
	long n = mpz_get_si(to_int(nV)->num);
	if(n < 0) return ERROR("NTHVALUE: index out of range!");

	void* result = eval_raw(car(a2), env);
	if(is_error(result)) return result;

	if(result && get_type(result) == TYPE_VALUES) {
	  void* i = car(result);
	  for(long k = 0; k < n && i; k++) i = cdr(i);
	  if(!i) return ERROR("NTHVALUE: index out of range!");
	  return car(i);
	}

	// Ordinary single value: only index 0 is in range.
	if(n != 0) return ERROR("NTHVALUE: index out of range!");
	return result;
      }
      break;

    case N_MULTIPLEVALUELIST:
      {
	// (MULTIPLEVALUELIST expr) -- expr via eval_raw for the same reason
	// as NTHVALUE. Returns the wrapped values as a plain list, or a
	// one-element list if expr was an ordinary single value.
	void* a1 = cdr(list);
	if(!a1 || !car(a1)) return ERROR("MULTIPLEVALUELIST requires 1 argument!");

	void* result = eval_raw(car(a1), env);
	if(is_error(result)) return result;

	if(result && get_type(result) == TYPE_VALUES) {
	  return car(result);
	}

	return cons(result, NULL);
      }
      break;

    case N_READ:
      return tread(env);
      break;
      
    case N_MAPMAKE:
      {
	// (MAPMAKE), (MAPMAKE pairs-list), or (MAPMAKE pairs-list comparator)
	// -- strictly positional: arg1 is always the pairs-list (a list of
	// (key . value) pairs, or NULL for none), arg2 is always the
	// comparator (a callable, or NULL/omitted for the default compare()).
	// There is no single-argument comparator-only form -- a comparator
	// with no pairs is (MAPMAKE NULL comparator). This mirrors what
	// PRINT shows for an existing tree -- (MAPMAKE '((1 . A) (2 . B))
	// comparator) or (MAPMAKE NULL comparator) -- so a printed tree can
	// be copy-pasted back in to reconstruct it.
	//
	// This is the ONLY place a tree's comparator is ever set -- once
	// make_rb_tree returns, every MAPADD call below (and every other
	// MAP* operation afterward) only ever reads it.
	void* args = cdr(list);

	if(args && cdr(args) && cdr(cdr(args))) {
	  return ERROR("MAPMAKE: too many arguments!");
	}

	void* pairs = NULL;
	void* comparator = NULL;

	if(args) {
	  pairs = eval(car(args), env);
	  if(is_error(pairs)) return pairs;

	  if(cdr(args)) {
	    comparator = eval(car(cdr(args)), env);
	    if(is_error(comparator)) return comparator;
	  }
	}

	if(comparator) {
	  ValueType t = get_type(comparator);
	  if(t != TYPE_NATIVE && t != TYPE_LAMBDA && t != TYPE_MACRO) {
	    return ERROR("MAPMAKE: comparator must be a function!");
	  }
	}

	if(pairs && !is_cons(pairs)) {
	  return ERROR("MAPMAKE: expected a list of (key . value) pairs!");
	}

	for(void* i=pairs; i; i=cdr(i)) {
	  // A (key . value) pair is just a cons cell -- value may itself be
	  // any type, including a list (which is also a cons chain, so this
	  // check must not reject that). We only need "is this a cons at
	  // all," not "is the cdr specifically non-cons."
	  if(!is_cons(car(i))) {
	    return ERROR("MAPMAKE: expected a (key . value) pair!");
	  }
	}

	cc tree = make_rb_tree(comparator);
	for(void* i=pairs; i; i=cdr(i)) {
	  void* pair = car(i);
	  mapadd(tree, car(pair), cdr(pair), env);
	}

	return tree;
      }
      break;

    case N_MAPADD:
      {

	void* tmp1 = cdr(list);
	if(!tmp1) {
	  return ERROR("ERROR: MAPADD requires 3 arguments!");
	}

	void* tmp2 = cdr(tmp1);
	if(!tmp2) {
	  return ERROR("ERROR: MAPADD requires 3 arguments!");
	}

	void* tmp3 = cdr(tmp2);
	if(!tmp3) {
	  return ERROR("ERROR: MAPADD requires 3 arguments!");
	}
	
	void* a = eval(car(tmp1), env);
	void* b = eval(car(tmp2), env);
	void* c = eval(car(tmp3), env);

	return mapadd(a, b, c, env);
      }
      break;
      
    case N_MAPGET:
      {
	void* tmp1 = cdr(list);
	if(!tmp1) {
	  return ERROR("ERROR: MAPGET requires 2 arguments!");
	}

	void* tmp2 = cdr(tmp1);
	if(!tmp2) {
	  return ERROR("ERROR: MAPGET requires 2 arguments!");
	}
	
	void* a = eval(car(tmp1), env);
	void* b = eval(car(tmp2), env);

	return mapget(a, b, env);
      }
      break;
      
    case N_MAPSET:
      {
	void* tmp1 = cdr(list);
	if(!tmp1) {
	  return ERROR("ERROR: MAPADD requires 3 arguments!");
	}

	void* tmp2 = cdr(tmp1);
	if(!tmp2) {
	  return ERROR("ERROR: MAPADD requires 3 arguments!");
	}

	void* tmp3 = cdr(tmp2);
	if(!tmp3) {
	  return ERROR("ERROR: MAPADD requires 3 arguments!");
	}
	
	void* a = eval(car(tmp1), env);
	void* b = eval(car(tmp2), env);
	void* c = eval(car(tmp3), env);

	return mapset(a, b, c, env);
      }
      break;

    case N_MAPDEL:
      {
	void* tmp1 = cdr(list);
	if(!tmp1) {
	  return ERROR("ERROR: MAPDEL requires 2 arguments!");
	}

	void* tmp2 = cdr(tmp1);
	if(!tmp2) {
	  return ERROR("ERROR: MAPDEL requires 2 arguments!");
	}
	
	void* a = eval(car(tmp1), env);
	void* b = eval(car(tmp2), env);

	return mapdel(a, b, env);
      }
      break;

    case N_MAP:
      {
	if(!cdr(list) || !car(cdr(list)) ||
	   !cdr(cdr(list))) {
	  return ERROR("MAP requires at least 2 arguments!");
	}

	void* func = eval(car(cdr(list)), env);
	if(is_error(func)) return func;

	void* lists = cdr(cdr(list));
	void* ret = NULL;
	void* lret = NULL;
	// then a dumb loop

	void* clists = NULL;
	void* tmp = NULL;
	for(void* i=lists; i; i=cdr(i)) {

	  void* e = eval(car(i), env);
	  if(is_error(e)) return e;
	  if(!is_cons(e)) return ERROR("Map requires lists... as of now...");

	  if(!clists) {
	    clists = cons(e, NULL);
	    tmp = clists;
	  }
	  else {
	    cdr(tmp) = cons(e, NULL);
	    tmp = cdr(tmp);
	  }
	}

	while(clists) {
	  // stripe across the lists and zip them up...
	  // and also the clists.
	  void* args = NULL;
	  tmp = NULL;
	  void* tmp2 = NULL;

	  for(void* i=clists; i; i=cdr(i)) {

	    // if car(i) is null then we end?
	    if(!car(i)) return ret;

	    if(!args) {
	      args = cons(car(car(i)), NULL);
	      tmp = args;

	      clists = cons(cdr(car(i)), NULL);
	      tmp2 = clists;
	    }
	    else {
	      cdr(tmp) = cons(car(car(i)),NULL);
	      tmp = cdr(tmp);

	      cdr(tmp2) = cons(cdr(car(i)), NULL);
	      tmp2 = cdr(tmp2);
	    }
	  }

	  // args are already-evaluated values -- apply directly, no re-eval.
	  void* a = apply_callable(func, args, ARGS_VALUES, env, env);
	  if(is_error(a)) return a;

	  if(!ret) {
	    ret = cons(a, NULL);
	    lret = ret;
	  }
	  else {
	    cdr(lret) = cons(a, NULL);
	    lret = cdr(lret);
	  }
	}

	return ret;
      }
      break;

    case N_REDUCE:
      {
	// (REDUCE func list initial)
	void* a1 = cdr(list);
	if(!a1 || !car(a1)) return ERROR("REDUCE requires 3 arguments!");
	void* a2 = cdr(a1);
	if(!a2 || !car(a2)) return ERROR("REDUCE requires 3 arguments!");
	void* a3 = cdr(a2);
	if(!a3 || !car(a3)) return ERROR("REDUCE requires 3 arguments!");

	void* func = eval(car(a1), env);
	if(is_error(func)) return func;

	void* target = eval(car(a2), env);
	if(is_error(target)) return target;
	if(!is_cons(target)) return ERROR("REDUCE requires a list!");

	void* acc = eval(car(a3), env);
	if(is_error(acc)) return acc;

	for(void* i=target; i; i=cdr(i)) {
	  acc = apply_callable(func, cons(acc, cons(car(i), NULL)), ARGS_VALUES, env, env);
	  if(is_error(acc)) return acc;
	}

	return acc;
      }
      break;

    case N_FILTER:
      {
	// (FILTER pred list)
	void* a1 = cdr(list);
	if(!a1 || !car(a1)) return ERROR("FILTER requires 2 arguments!");
	void* a2 = cdr(a1);
	if(!a2 || !car(a2)) return ERROR("FILTER requires 2 arguments!");

	void* pred = eval(car(a1), env);
	if(is_error(pred)) return pred;

	void* target = eval(car(a2), env);
	if(is_error(target)) return target;
	if(!is_cons(target)) return ERROR("FILTER requires a list!");

	void* ret = NULL;
	void* last = NULL;

	for(void* i=target; i; i=cdr(i)) {
	  void* keep = apply_callable(pred, cons(car(i), NULL), ARGS_VALUES, env, env);
	  if(is_error(keep)) return keep;

	  if(is_true(keep)) {
	    if(!ret) {
	      ret = cons(car(i), NULL);
	      last = ret;
	    }
	    else {
	      cdr(last) = cons(car(i), NULL);
	      last = cdr(last);
	    }
	  }
	}

	return ret;
      }
      break;
      
    case N_LET:
      {
	if(!cdr(list) || !car(cdr(list))) {
	  return ERROR("LET requires 2 arguments!");
	}

	if(!cdr(cdr(list)) || !car(cdr(cdr(list)))) {
	  return ERROR("LET requires 2 arguments!");
	}
	
	void* vars = car(cdr(list));

	void* frame = NULL;
	
	// First loop over all the values and eval the values...
	// Add in the values as we go, it doesn't cost us anything...
	void* newenv = cons(car(env), cdr(env)); 
	// loop over the variables (arg1)
	for(void* i=vars; i; i=cdr(i)) {

	  void* pair = car(i);
	  void* name = car(pair);
	  void* value = car(cdr(pair));
	  
	  if(!frame) {
	    value = eval(value, env);
	  }
	  else {
	    value = eval(value, newenv);
	  }

	  if(is_error(value)) {
	    return value;
	  }

	  frame = cons(cons(name, value), frame);
	  
	  car(newenv) = cons(frame, car(env));
	  
	}
	  
	void* ret = NULL;
	
	
	void* code = cdr(cdr(list));
	// loop over the code...
	for(void* i=code; i; i=cdr(i)) {

	  void* tmp = eval(car(i), newenv);;

	  if(is_error(tmp)) return tmp;

	  ret = tmp;
	}

	return ret;
      }
      break;

    case N_PROGN:
      {
	void* ret = NULL;
	void* code = cdr(list);
	// loop over the code...
	for(void* i=code; i; i=cdr(i)) {

	  void* tmp = eval(car(i), env);;

	  if(is_error(tmp)) return tmp;

	  ret = tmp;
	}

	return ret;
      }
      break;

    case N_PROG1:
      {
	if(!cdr(list) || !car(cdr(list))) return NULL;

	void* ret = eval(car(cdr(list)), env);
	if(is_error(ret)) return ret;

	void* code = cdr(cdr(list));
	// loop over the code...
	for(void* i=code; i; i=cdr(i)) {

	  void* tmp = eval(car(i), env);;

	  if(is_error(tmp)) return tmp;
	}

	return ret;
      }
      break;
  
    case N_LAMBDA:

      // Lambda is set up as (lambda (args) code) 
      // but I create a (TYPE_LAMBDA closure args code)
      // let's just try storing the env itself. 
      if(!cdr(list) || !car(cdr(list))) {
	return ERROR("LAMBDA requires 2 arguments!");
      }
      
      if(!cdr(cdr(list)) || !car(cdr(cdr(list)))) {
	return ERROR("LAMBDA requires 2 arguments!");
      }

      void* e = butlast(car(env));
      return create_lambda(e, cons(car(cdr(list)), cdr(cdr(list))));
      break;

    case N_MACRO:

      // Same shape as LAMBDA -- (TYPE_MACRO closure args code). At call
      // time, a macro's params bind to the caller's RAW unevaluated forms
      // (never values), the body runs to produce an expansion form, and
      // that expansion is then eval'd in the CALLER's environment -- this
      // is simple, unhygienic, textual-expansion-style substitution (no
      // gensym); symbol capture is possible and is an accepted limitation.
      if(!cdr(list) || !car(cdr(list))) {
	return ERROR("MAC requires 2 arguments!");
      }

      if(!cdr(cdr(list)) || !car(cdr(cdr(list)))) {
	return ERROR("MAC requires 2 arguments!");
      }

      {
	void* e2 = butlast(car(env));
	return create_macro(e2, cons(car(cdr(list)), cdr(cdr(list))));
      }
      break;

    case N_CAT:
      {
	void* args = cdr(list);
      
	if(!args) {
	  return to_string("");
	}

	void* ret = NULL;
	cc next = NULL;
      
      for(void* i=cdr(list); i; i=cdr(i)) {
	void* tmp = eval(car(i), env);
	
	if(ret) {
	  cc c = cons(tmp, NULL);
	  next->cdr = c;
	  next = c;
	}
	else {
	  
	  ret = cons(tmp, NULL);
	  next = ret;
	}
      }
      args = ret;
      
      int slen = 0;
      for(void* o = args; o != NULL; o = cdr(o)) {
	switch(get_type(car(o))) {
	case TYPE_TRUE:
	case TYPE_NULL:
	  slen += 4;
	  break;
	  
	case TYPE_SYMBOL:
	case TYPE_STRING:
	  slen += to_string(car(o))->size;
	  break;
	  
	case TYPE_RESIZABLE_STRING:
	  slen += ((resizable_string_type*) (car(o)))->len;
	  break;
	  
	default:

	  
	  
	  return ERROR("CAT only works with strings and symbols types!");
	}
      }

      int p = 0;
      string_type* newstr = create_string_type(slen, TYPE_STRING);

      for(void* o = args; o != NULL; o = cdr(o)) {
	switch(get_type(car(o))) {
	case TYPE_TRUE:
	  newstr->str[p++] = 'T';
	  newstr->str[p++] = 'R';
	  newstr->str[p++] = 'U';
	  newstr->str[p++] = 'E';
	  break;

	  
	case TYPE_NULL:
	  newstr->str[p++] = 'N';
	  newstr->str[p++] = 'U';
	  newstr->str[p++] = 'L';
	  newstr->str[p++] = 'L';
	  break;
	  
	case TYPE_SYMBOL:
	case TYPE_STRING:
	  {
	    
	    string_type* ostr = to_string(car(o));
	    for(int i=0; i < ostr->size; i++) {
	      newstr->str[p] = ostr->str[i];
	      p++;
	    }
	  }
	  break;
	  
	case TYPE_RESIZABLE_STRING:
	  printf("copyint resizeable string\n");
	  resizable_string_type* rstr = ((resizable_string_type*) car(o));
	  for(int i=0; i<rstr->len; i++) {
	      newstr->str[p] = rstr->str[i];
	      p++;
	    }
	  break;

	  
	}
      }
      newstr->str[p] = '\0';
      
      return newstr;
      break;
      }
    default:
      
      return ERROR("Unknown native int function!!!\n");
    }

  case TYPE_LAMBDA:
    return apply_callable(car(list), cdr(list), ARGS_RAW_EVAL, env, env);
    break;
    
  case TYPE_CNR:
    {
      if(!cdr(list)) {
	return ERROR("EVAL requires ONE argument!\n");
      }
      string_type* str = to_string(car(o));
      char* cstr = str->str;
      void* ret = car(cdr(list));

      for(int i=0; cstr[i]; i++) {

	switch(cstr[i]) {
	case 'a':
	case 'A':

	  if(!ret) return ERROR("CAR on NULL!");
	  ret = car(ret);
	  break;

	case 'd':
	case 'D':
	  if(!ret) return ERROR("CDR on NULL!");
	  ret = cdr(ret);
	  break;

	default:
	  return ERROR("Unknown character in CN+R!");
	  break;
	}

	if(is_error(ret)) return ret;
      }

      return ret;
    }
    break;

    case TYPE_NPROG:
      {
	void* ret = NULL;
	int n = ((nprog_type*) car(list))->n;

	void* code = cdr(list);
	// loop over the code...
	int x = 1;
	for(void* i=code; i; i=cdr(i)) {

	  void* tmp = eval(car(i), env);;

	  if(is_error(tmp)) return tmp;

	  if(x == n) ret = tmp;
	  x++;
	}

	if(n > (x - 1)) return ERROR("Not enough values for nprog value!!!");

	return ret;
      }
      break;

  case TYPE_NATIVE:
    return apply_callable(o, cdr(list), ARGS_RAW_EVAL, env, env);
    break;

  case TYPE_SYMBOL:
    {
      void* f = eval(o, env);
      if(is_error(f)) return f;

      if(is_type(f, TYPE_MACRO)) {
	void* expansion = apply_callable(f, cdr(list), ARGS_RAW_NOEVAL, env, env);
	if(is_error(expansion)) return expansion;
	return eval(expansion, env);
      }

      return apply_callable(f, cdr(list), ARGS_RAW_EVAL, env, env);
    }
    break;

  default:

    printf("This type doesn't have a function handler eval_list (type = %s)!!!\n", return_type_c_string(o));
    break;
  }
  
  return NULL;
}
