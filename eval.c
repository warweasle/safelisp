#include <stdlib.h>
void (*old_free)(void*) = free;
#include "eval.h"

void* eval(void* list, void* env) {

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

  case TYPE_SYMBOL:
    {

      // I think this should be my problem!!!!!
      // I need to treat this like a (symbol item item...)
      // Reuse the native int function as a template
      
      for(void* i=car(env); i; i=cdr(i)) {

	switch(get_type(car(i))) {

	case TYPE_RB_TREE:
	  {
	    void* found = mapget(car(i), list);
	    if(found) return found;
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
	  ERROR("Set found an issue with the environment!!!\n");
	  break;
	}
      }

      return ERROR("Could not find symbol!");  
    }
    break;
      
    
  default:
    //printf("DEFAULT\n");
    return list;
  }
  
  return list;
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

      void* args = NULL;
      void* last = NULL;
      
      for(void* i=cdr(list); i; i=cdr(i)) {

	void* a = eval(car(i), env);
	if(is_error(a)) return a;
	
	if(args == NULL) {
	  args = cons(a, NULL);
	  last = args;
	}
	else {
	  cdr(last) = cons(a, NULL);
	  last = cdr(last);
	}
      }

      return eval(cons(f, args), env);
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

    case N_TO_STRING:
      {
	char *buf = NULL;
	size_t size = 0;

	if(!cdr(list)) {
	  return ERROR("TO-STRING requires ONE argument!\n");
	}

	void* p = eval(car(cdr(list)), env);
	FILE *f = open_memstream(&buf, &size);
	
	print(f, p, 10);
	
	fflush(f);
	fclose(f);   // IMPORTANT: finalizes buffer
	
	void* ret = create_string_type_from_string(buf, TYPE_STRING);
	old_free(buf);
	return ret;
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
	      void* found = mapget(car(i), name);
	      if(is_error(found)) return found;

	      if(found) {
		value = eval(value, env);
		if(is_error(value)) return value;

		cdr(found) = value;
	      }
	      else {
		mapset(car(i), name, value);
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
		/// int and rational ... so a rational
		
		rational_type* ar = create_rational_type(); 
	     	mpq_set_z(ar->num, to_int(b)->num);
	        mpq_add(ar->num, ar->num, to_rational(b)->num);
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
		rational_type* result = create_rational_type();
		mpq_set_z(result->num, to_int(b)->num);
		mpq_add(result->num, to_rational(a)->num, result->num);
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
	      rational_type* ret = create_rational_type(0);
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
	      
	      rational_type* ret = create_rational_type(0);
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
	      
	      rational_type* ret = create_rational_type(0);
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

      void* oldenv = NULL;
      oldenv = cdr(env);
      
      void* start = cdr(list);
      if(!start || !car(start)) {
	return ERROR("LOOP requires at least one argument!");
      }

      cdr(env) = cons(cons(create_symbol("*BREAK*"), NULL), cdr(env));
      
      void* i = start;
      void* ret = cdr(cassoc("*BREAK*", cdr(env)));
      while(!ret) {
	eval(car(i), env);
	ret = cdr(cassoc("*BREAK*", cdr(env)));
	
	if(cdr(i)) i = cdr(i);
	else       i = start;
      }
      cdr(env) = oldenv;
      
      return ret;
      break;

    case N_READ:
      return tread(env);
      break;
      
    case N_MAPMAKE:
      return make_rb_tree();
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

	return mapadd(a, b, c);
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

	return mapget(a, b);
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

	return mapset(a, b, c);
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

	return mapdel(a, b);
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
      
    default:
      
      return ERROR("Unknown native int function!!!\n");
    }

  case TYPE_LAMBDA:
    {

      void* lambda = car(list);
      void* closure = car(lambda);
      void* args = car(cdr(lambda));
      void* vals = cdr(list);

      void* oldenv = car(env);
      
      // first deal with the closure...
      void* newenv = NULL;
      void* l = NULL;
      if(closure) {
	l = last(closure);
	cdr(l) = car(car(env));
	newenv = cons(l, car(env));
      }
      else {
	newenv = env;
      }
      
      if(!args && vals) return ERROR("Sent args to a function and accepts none!");

      void* nextFrame = NULL;

      if(car(args)) {
	for(void* i = args; i; i=cdr(i)) {

	  if(!vals) {
	    return ERROR("NOT ENOUGH ARGUMENTS FOR THE FUNCTION!!!");
	  }

	  void* val = eval(car(vals), newenv);
	  nextFrame = cons(cons(car(i), val), nextFrame);
	  vals = cdr(vals);
	}

	// set the new env with the lambda list...
	car(newenv) = cons(nextFrame, car(newenv));
      }
      
      // run the code with the new env
      void* ret = NULL;
      for(void* i=cdr(cdr(lambda)); i; i=cdr(i)) {

	ret = eval(car(i), newenv);

	if(is_error(ret)) {
	  car(env) = oldenv;
	  return ret;
	}
      }

      // reset the end so we don't mess up the closure.
      if(closure) {
	cdr(l) = NULL;
      }

      car(env) = oldenv;
      return ret;
    }

    return NULL;
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

  case TYPE_SYMBOL:
    {
      // ok, we need to do more...
      void* ret = eval(o, env);
      return eval(cons(ret, cdr(list)), env);
    }
    break;
        
  default:

    printf("This type doesn't have a function handler eval_list (type = %s)!!!\n", return_type_c_string(o));
    break;
  }
  
  return NULL;
}
