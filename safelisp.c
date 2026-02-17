#include <stdlib.h>
void (*old_free)(void*) = free;
#include "safelisp_parser.tab.h" // Include Bison-generated headers, which might vary by your setup
#include "safelisp_parser.yy.h"
#include "safelisp.h"

ValueType ttrue = TYPE_TRUE;

void *gmp_gc_malloc(size_t size) {
  return GC_malloc(size);
}

void *gmp_gc_realloc(void *ptr, size_t old_size, size_t new_size) {
  (void)old_size; // This explicitly marks the unused parameter to avoid warnings
  return GC_realloc(ptr, new_size);
}

void gmp_gc_free(void *ptr, size_t size) {
  (void)size; // Mark unused, as GC_FREE doesn't use size
  GC_free(ptr);
}

// Initialize a scanner instance for Flex
yyscan_t scanner;
  
void* init_safelisp(FILE* input, FILE* output) {
  GC_INIT();
  mp_set_memory_functions(GC_malloc, gmp_gc_realloc, gmp_gc_free);

  ttrue = TYPE_TRUE;
  
  void* ret = NULL;
  ret = cons(cons(create_symbol("*INPUT*"), create_pointer_type(input)), ret);
  ret = cons(cons(create_symbol("*OUTPUT*"), create_pointer_type(output)), ret);
  
  return cons(NULL, ret);
}

// Set the event flag
void set_event_flag(void* value) {
  *((int*) value) |= EVENT_FLAG;
}
  
// Clear the event flag
void clear_event_flag(void* value) {
  *((int*) value) &= ~EVENT_FLAG;
}
  
// Check if the event flag is set
int is_event_flag_set(void* value) {
  return (*((int*) value) & EVENT_FLAG) != 0;
}
  
// Set the red/black flag
void set_red_black_flag(void* value) {
  *((int*) value) |= RED_BLACK_FLAG;
}

// Clear the red/black flag
void clear_red_black_flag(void* value) {
  *((int*) value) &= ~RED_BLACK_FLAG;
}

// Check if the red/black flag is set
int is_red_black_flag_set(void* value) {
  return (*((int*) value) & RED_BLACK_FLAG) != 0;
}

int is_true(void* o) {

  if(!o) return 0;
  
  ValueType type = get_type(o);

  switch(type) {

  case TYPE_INT:
    if(mpz_cmp_ui(to_int(o)->num, 0) == 0) return 0;
    else return -1;

  case TYPE_FLOAT: 
    if(mpf_cmp_ui(to_float(o)->num, 0) == 0) return 0;
    else return -1;

    // I don't think this is the best idea.
    /* case TYPE_CONS: */
    /*   if(car(o) || cdr(o)) return -1; */
    /*   else return 0; */
    
  default:
  
  }
  return -1;
}

cc cons(void* car, void* cdr) {
  cc ret = (cc) GC_malloc(sizeof(cons_cell));

  ret->type = TYPE_CONS;
  ret->car = car;
  ret->cdr = cdr;  
  return ret;
}

cc make_cnr(void* cnr) {
  cc ret = (cc) GC_malloc(sizeof(cons_cell));

  ret->type = TYPE_CNR;
  ret->car = cnr;
  ret->cdr = NULL;
  return ret;
}

cc make_rb_tree() {

  cc ret = (cc) GC_malloc(sizeof(cons_cell));

  ret->type = TYPE_RB_TREE;
  return ret;
}

cc error(void* car, void* cdr) {
  cc ret = (cc) GC_malloc(sizeof(cons_cell));

  ret->type = TYPE_ERROR;
  ret->car = car;
  ret->cdr = cdr;  
  return ret;
}

int is_list(void* list) {
  return is_cons(list);
}

int list_length(void* list) {
  int length = 0;
  void* current = list;
  
  if(!is_cons(list)) {
    return -1;
  }
  
  while (current) {
    // current better be a cons
    if(!is_cons(current)) {
      return -1; 
    }

    length++;
    current = cdr(current); // Move to the next element in the list

  }
  
  // If the list ends with an object that is not a cons cell, and it's not NULL,
  // you might want to count it as well, depending on your definition of "length".
  // Remove the following line if you don't want to count the non-cons-cell ending as part of the length.
  if (current != NULL) {
    length++;
  }

  return length;
}

cc last(void* lst) {

  if(!is_cons(lst)) {
    printf("ERROR, You sent a non-list to LAST!!!\n");
    return NULL;
  }

  cons_cell* i;
  for(i=lst; i != NULL && get_type(i) == TYPE_CONS && cdr(i); i=cdr(i)) {/* Nothing here on purpose */}

  return i;
  
}

cc create_quotetype(ValueType Type, void* car) {

  cc ret = (cc) GC_malloc(sizeof(cons_cell));
  ret->type = Type;
  ret->car = car;
  return ret;
}


char_type* create_char_type(char c) {
  char_type* o = (char_type*)GC_malloc(sizeof(char_type));
  if(!o) return NULL; // Check for allocation failure
  o->type = TYPE_CHAR;
  o->c = c;
  return o;
}

void* create_true_type() {
  return &ttrue;
}

void* create_false_type() {

  return NULL;
}


string_type* create_string_type(size_t len, ValueType Type) {
  string_type* sym = (string_type*)GC_malloc(sizeof(string_type) + len + 1); // +1 for null terminator
  if (!sym) return NULL; // Check for allocation failure
  sym->type = Type;
  sym->size = len;
  return sym;
}

string_type* create_string_type_and_copy(size_t len, const char* str, ValueType Type) {
  size_t strLen = strlen(str);
  if (strLen < len) {
    len = strLen; // Adjust len to the actual string length if it's shorter
  }
  string_type* sym = (string_type*)GC_malloc(sizeof(string_type) + len + 1); // +1 for null terminator
  if (!sym) return NULL; // Check for allocation failure

  sym->type = Type;
  strncpy(sym->str, str, len+1); // Copy up to len characters
  //sym->str[len] = '\0'; // Ensure null termination

  return sym;
}

string_type* create_string_type_from_string(const char* str, ValueType Type) {
  return create_string_type_and_copy(strlen(str), str, Type);
}


// This is here so I can cache symbols...
void add_symbol(string_type* symbol) {
  return;
}

string_type* create_symbol(const char* str) {

  return create_string_type_from_string(str, TYPE_SYMBOL); 
}

string_type* create_symbol_and_copy(size_t len, const char* str) {
  return create_string_type_and_copy(len, str, TYPE_SYMBOL);
}

resizable_string_type* create_resizable_string_type_and_copy(size_t len, const char* str, ValueType Type) {
  size_t strLen = strlen(str);
  if (strLen < len) {
    len = strLen; // Adjust len to the actual string length if it's shorter
  }
  resizable_string_type* sym = (resizable_string_type*)GC_malloc(sizeof(resizable_string_type) + len + 1); // +1 for null terminator
  if (!sym) return NULL; // Check for allocation failure

  sym->type = Type;
  strncpy(sym->str, str, len+1); // Copy up to len characters
  //sym->str[len] = '\0'; // Ensure null termination

  return sym;
}

resizable_string_type* create_resizable_string_type(size_t len, ValueType Type) {
  resizable_string_type* sym = (resizable_string_type*)GC_malloc(sizeof(resizable_string_type));
  if (!sym) return NULL; // Check for allocation failure
  sym->type = Type;
  sym->len = 1 > len ? 1 : len;
  sym->pos = 0;
  sym->str = (char*)GC_malloc(sym->len); // +1 for null terminator
  if (sym->str) {
    sym->str[0] = '\0'; // Initialize empty string
  } else {
    GC_free(sym); // Clean up if string allocation fails
    return NULL;
  }
  return sym;
}
	     
pointer_type* create_pointer_type(void* p) {
  pointer_type* ret = (pointer_type*)GC_malloc(sizeof(pointer_type)); 
  if (!ret) return NULL; // Check for allocation failure
  ret->type = TYPE_POINTER;
  ret->p = p;

  return ret;
}
	     
char* resize_string(char* str, size_t size) {

  char* ret;
  if(str) {
    ret = GC_realloc(str, size);
  }
  else {
    ret = GC_malloc(size);
  }
  
  if(!ret) {
    printf("Out of memory for resizable string!\n");
    return NULL;
  }

  return ret;
}

cc create_lambda(void* args, void* code) {
  cc ret = (cc) GC_malloc(sizeof(cons_cell));

  ret->type = TYPE_LAMBDA;
  ret->car = args;
  ret->cdr = code;  
  return ret;
}

resizable_string_type* resize_resizable_array(resizable_string_type* arr, size_t size) {
  char* newStr = resize_string(arr->str, size);
  if(!newStr) {
    return NULL;
  }

  arr->str = newStr;
  arr->len = size;
  return arr;  
}

string_type* create_string_type_from_resizable_string(resizable_string_type* resizeable) {
  return create_string_type_and_copy(resizeable->pos, resizeable->str, TYPE_STRING);
}

resizable_string_type* putch_resizable_array(resizable_string_type* arr, char c) {
  
  if(!is_type(arr, TYPE_RESIZABLE_STRING)) {
	
    printf("Error, not a resizeable string!\n");
    return NULL;
  }
  
  // Is there enough space? 
  if(arr->pos >= arr->len - 2) {
	
    // Gotta make some room...
    resize_resizable_array(arr, arr->len * 2);
  }

  if(arr->str != NULL) {
  
    arr->str[arr->pos] = c;
    arr->pos++;
    arr->str[arr->pos] = '\0';
  }
  
  return arr;
}

resizable_string_type* putstr_resizable_array(resizable_string_type* arr, char* s) {

  if(!is_type(arr, TYPE_RESIZABLE_STRING)) {
	
    printf("Error, not a resizeable string!\n");
    return NULL;
  }

  size_t strl = strlen(s);

  // if this is the first item then add an extra space for the NULL.
  if(arr->pos == 0) strl += 1;
  size_t totalRequired = arr->pos + strl;
						 
  if(totalRequired >= arr->len - 1) {

    // Gotta make some room...
		
    arr->len = totalRequired * totalRequired; 
    arr->str = resize_string(arr->str, arr->len);
  }

  if(arr->str != NULL) {
  
    strncpy(arr->str + arr->pos, s, strl +1);
    arr->pos = totalRequired;
  }
  
  return arr;
}

int string_compare(const void* a, const void* b) {
   
  /* if (!is_str(a) || !is_str(b)) { */
  /*   printf("is this erroring?\n"); */
  /*   return 0; // Consider returning 0 or handle the error as appropriate */
  /* } */

  const string_type* str1 = (const string_type*)a;
  const string_type* str2 = (const string_type*)b;  

  int ret = strcmp(str1->str, str2->str);
    
  // Compare the strings
  return ret;
}

int raw_string_compare(const void* a, const void* b) {
  const string_type* str1 = (const string_type*)a;
  const char* str2 = (const char*)b;

  // Ensure both are valid strings using is_str
  if (!is_str(str1)) {
    return 0; // Consider returning 0 or handle the error as appropriate
  }

  printf("Comparing: %s vs %s\n", str1->str, str2);
  
  // Compare the strings
  return strcmp(str1->str, str2);
}


char_type* create_native_int_type(nativeType type) {

  char_type* ret = (char_type*) GC_malloc(sizeof(char_type));
  ret->type = TYPE_NATIVE_INT;
  ret->c = type;
  return ret;
}


int_type* create_int_type(double i) {
  int_type* ret = GC_malloc(sizeof(int_type));

  ret->type = TYPE_INT;
  mpz_init(ret->num);
  mpz_set_d(ret->num, i);
  return ret;
}

float_type* create_float_type() {
  float_type* ret = GC_malloc(sizeof(float_type));

  ret->type = TYPE_FLOAT;
  mpf_init(ret->num);
  return ret;
}

rational_type* create_rational_type() {
  rational_type* ret = GC_malloc(sizeof(rational_type));

  ret->type = TYPE_RATIONAL;
  mpq_init(ret->num);
  
  return ret;
}

int compare_lists(cc a, cc b) {

  if(a == b) return 0;

  printf("Not working yet!\n");
  return -1;
}

int compare(void* a, void* b) {

  if(a == b) return 0;
  
  ValueType at = get_type(a);
  ValueType bt = get_type(b);

  if(at != bt) return at - bt;

  switch(at) {
  
  case TYPE_NULL:
    return 0; 
    break;
    
  case TYPE_CONS:
    {
      cc ac = to_cons(a);
      cc bc = to_cons(b);
      return compare_lists(ac, bc);
    }
    break;
    
  case TYPE_SYMBOL:
  case TYPE_STRING:
    return string_compare(a, b);
    break;

  case TYPE_TRUE:
  case TYPE_QUOTE:
  case TYPE_BACKTICK:
  case TYPE_COMMA:
  case TYPE_SPLICE:
  
    return 0;
    break;

  case TYPE_INT:
    return mpz_cmp(to_int(a)->num, to_int(b)->num);
    break;
    
  case TYPE_FLOAT:
    return mpf_cmp(to_float(a)->num, to_float(b)->num);
    break;

  case TYPE_RATIONAL:
    return mpq_cmp(to_rational(a)->num, to_rational(b)->num);
    break;

  case TYPE_NATIVE_INT:
    return to_char(a)->c - to_char(b)->c;
    break;
    
  case TYPE_CHAR:
  case TYPE_RESIZABLE_STRING:
  case TYPE_NATIVE:
  case TYPE_LAMBDA:
  case TYPE_RAW:
  case TYPE_INT8:
  case TYPE_UINT8:
  case TYPE_FLOAT8:
  case TYPE_DOUBLE8:
  case TYPE_LONG_DOUBLE8:
  case TYPE_INT16:
  case TYPE_UINT16:
  case TYPE_FLOAT16:
  case TYPE_DOUBLE16:
  case TYPE_LONG_DOUBLE16:
  case TYPE_INT32:
  case TYPE_UINT32:
  case TYPE_FLOAT32:
  case TYPE_DOUBLE32:
  case TYPE_LONG_DOUBLE32:
  case TYPE_INT64:
  case TYPE_UINT64:
  case TYPE_FLOAT64:
  case TYPE_DOUBLE64:
  case TYPE_LONG_DOUBLE64:
  case TYPE_INT128:
  case TYPE_UINT128:
  case TYPE_FLOAT128:
  case TYPE_DOUBLE128:
  case TYPE_LONG_DOUBLE128:
  case TYPE_CHAR_ARRAY:
  case TYPE_POINTER:
  case TYPE_RB_TREE:
  case TYPE_ERROR:

    return 0;
    break;
      
  default:
  }
  

  return 0;
}

void* equal(void* a, void* b) {
  if(compare(a, b) == 0) {
    return create_true_type();
  }
  else {
    return NULL;
  }
}

char* return_type_c_string(void* o) {

  ValueType type = get_type(o);

  switch(type) {

  case TYPE_NULL:
    return "NULL";
  case TYPE_CONS:
    return "CONS";
  case TYPE_SYMBOL:
    return "SYMBOL";
  case TYPE_STRING:
    return "STRING";
  case TYPE_TRUE:
    return "TRUE";
  case TYPE_QUOTE:
    return "QUOTE";
  case TYPE_BACKTICK:
    return "BACKTICK";
  case TYPE_COMMA:
    return "COMMA";
  case TYPE_SPLICE:
    return "SPLICE";
  case TYPE_INT:
    return "INTEGER";
  case TYPE_FLOAT:
    return "FLOAT";
  case TYPE_RATIONAL:
    return "RATIONAL";
  case TYPE_NATIVE_INT:
    return "NATIVE_FUNC";
  case TYPE_POINTER:
    return "POINTER";
  case TYPE_RB_TREE:
    return "RB_TREE";
  case TYPE_ERROR:
    return "ERROR";
  case TYPE_CHAR:
    return "CHAR";
  case TYPE_RESIZABLE_STRING:
    return "RESIZABLE_STRING";
  case TYPE_NATIVE:
    return "NATIVE_POINTER";
  case TYPE_LAMBDA:
    return "LAMBDA";
  case TYPE_RAW:
    return "RAW";
  case TYPE_INT8:
    return "INT8";
  case TYPE_UINT8:
    return "UINT8";
  case TYPE_FLOAT8:
    return "FLOAT8";
  case TYPE_DOUBLE8:
    return "DOUBLE8";
  case TYPE_LONG_DOUBLE8:
  case TYPE_INT16:
  case TYPE_UINT16:
  case TYPE_FLOAT16:
  case TYPE_DOUBLE16:
  case TYPE_LONG_DOUBLE16:
  case TYPE_INT32:
  case TYPE_UINT32:
  case TYPE_FLOAT32:
  case TYPE_DOUBLE32:
  case TYPE_LONG_DOUBLE32:
  case TYPE_INT64:
  case TYPE_UINT64:
  case TYPE_FLOAT64:
  case TYPE_DOUBLE64:
  case TYPE_LONG_DOUBLE64:
  case TYPE_INT128:
  case TYPE_UINT128:
  case TYPE_FLOAT128:
  case TYPE_DOUBLE128:
  case TYPE_LONG_DOUBLE128:
  case TYPE_CHAR_ARRAY:
    
  default:
    return "UNKNOWN";
    
  }
  
}

void* return_type(void* o) {
  
  return create_symbol(return_type_c_string(o));
}

void* assoc(void* item, void* list) {
  if(list == NULL) return NULL;

  if(car(list)) {
    if (car(car(list))) 
      {
	if(equal(item, car(car(list)))){
	  return car(list);	  
	}	
      }
  }
  
  return assoc(item, cdr(list));
}

void* cassoc(char* str, void* list) {

  if(list == NULL) return NULL;

  if(car(list) && 
     car(car(list)))
    {

      string_type* target = to_string(car(car(list)));
      
      if(strcmp(str, target->str) == 0) {
	return car(list);
      }
    }

  return cassoc(str, cdr(list));
}

void* eval(void* list, void* env) {

  ValueType type = get_type(list);
  
  switch(type) {

  case TYPE_CONS:
    return eval_list(list, env);

  case TYPE_QUOTE:
    if(!car(list)) {
      printf("Error: quote requires something after it!\n");
      return NULL;
    }
    
    return car(list);
    break;

  case TYPE_SYMBOL:

    {
      void* found = assoc(list, car(env));
      if(found && cdr(found)) {
	return cdr(found);
      }
      else {
	return ERROR("Could not find symbol!"); 
      }
    }
    break;
    
  default:
    return list;
  }
  
  return list;
}

void* eval_rest(void* list, void* env) {

  void* o = car(to_cons(list));
  ValueType type = get_type(o);
  
  switch(type) {
  case TYPE_CONS:
    return eval_list(list, env);
    break;
    
  case TYPE_SYMBOL:
    // look up symbols...
    
    //break;

  default:

    return list;
    
    
  }

  
  return NULL;
}

void* eval_list(void* list, void* env) {
  
  void* o = car(list);
  ValueType type = get_type(o);
  
  //printf("Type = %i or %s\n", type, return_type_c_string(o));
  
  switch(type) {
  case TYPE_CONS:
    printf("Function calling not yet available!!!\n");
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
	  printf("ERROR: nothing to IF!");
	  return NULL;
	}

	if(!truth) {
	  printf("Nothing to execute in IF statement!\n");
	  return NULL;
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
	printf("TYPE requires ONE argument!\n");
	return NULL;
      }

      if(cdr(cdr(list))) {
	printf("TYPE requires only ONE argument!\n");
	return NULL;
      }
      
      return return_type(eval(car(cdr(list)), env));
      break;
      
    case N_NOT:
      if(!cdr(list)) {
	printf("NOT requires ONE argument!\n");
	return NULL;
      }

      if(cdr(cdr(list))) {
	printf("NOT requires only ONE argument!\n");
	return NULL;
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
	  printf("ERROR: APPEND requires TWO arguments!\n");
	  return NULL;
	}

	cc tmp = cdr(list); 
	
	cc l = last(car(tmp));

	l->cdr = car(cdr(tmp));

	return car(tmp);
      }
      break;
      
    case N_ASSOC:

      {
	if(!cdr(list) || !cdr(cdr(list))) {
	  printf("ERROR: ASSOC requires at least TWO arguments!\n");
	  return NULL;
	}
	
	void* item = eval(car(cdr(list)), env);
	void* db = eval(car(cdr(cdr(list))), env);

	return assoc(item, db);
      }
      break;
      
    case N_EVAL:
      if(!cdr(list) || !car(cdr(list))) {
	printf("EVAL requires ONE argument!\n");
	return NULL;
      }

      return eval(eval(car(cdr(list)), env), env);
      break;
      
    case N_EQL:
      {
	if(!cdr(list)) {
	  printf("ERROR: EQL requires 2 arguements!");
	  return NULL;
	}
	if(!cdr(cdr(list))) {
	  printf("ERROR: EQL requires 2 arguements!");
	  return NULL;
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
	  printf("TO-STRING requires ONE argument!\n");
	  return NULL;
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
	void* found = assoc(name, car(env));
		
	if(!found) {
	  car(env) = cons(cons(name, value), car(env));
	}
	else {
	  cdr(found) = value;
	}
	return value;
      }
      break;

    case N_WHILE:
      {
	void* pred = car(cdr(list));
	void* code = cdr(cdr(list));
	
	if(!pred) {
	  printf("ERROR: nothing to WHILE!\n");
	  return NULL;
	}

	if(!code) {
	  printf("Nothing to execute in WHILE statement!\n");
	  return NULL;
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

	void* a = car(cdr(list));
	
	for(cc i = cdr(cdr(list)); i; i=cdr(i)) {

	  void* b = car(i);
	  
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
		mpq_canonicalize(ar->num);
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
		float_type* ar = create_float_type(); 
	     	mpf_set_q(ar->num, to_rational(b)->num);
	        mpf_add(ar->num, to_float(a)->num, ar->num);
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
	
        return a;
      }	
      break;

    case N_SUB:
      
      break;

    case N_MULT:
      {
	if(!car(cdr(list)) || !cdr(cdr(list)) || !car(cdr(cdr(list)))) {
	  return ERROR("ADD requires at least two arguments!");
	}

	void* a = car(cdr(list));
	
	for(cc i = cdr(cdr(list)); i; i=cdr(i)) {

	  void* b = car(i);
	  
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

	    printf("is this running?\n");
	    
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
	
        return a;
      }	
      break;

    case N_DIV:
      
      break;


      
    case N_BREAK:
      {
	
	void* pair = cassoc("*BREAK*", cdr(env));
	if(!pair) {
	  return ERROR("BREAK outside of LOOP!!!");
	}

	//print(stdout, list, 10);
	
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

      void* oldenv = cdr(env);
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
	  printf("ERROR: MAPADD requires 3 arguments!");
	  return NULL;
	}

	void* tmp2 = cdr(tmp1);
	if(!tmp2) {
	  printf("ERROR: MAPADD requires 3 arguments!");
	  return NULL;
	}

	void* tmp3 = cdr(tmp2);
	if(!tmp3) {
	  printf("ERROR: MAPADD requires 3 arguments!");
	  return NULL;
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
	  printf("ERROR: MAPGET requires 2 arguments!");
	  return NULL;
	}

	void* tmp2 = cdr(tmp1);
	if(!tmp2) {
	  printf("ERROR: MAPGET requires 2 arguments!");
	  return NULL;
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
	  printf("ERROR: MAPADD requires 3 arguments!");
	  return NULL;
	}

	void* tmp2 = cdr(tmp1);
	if(!tmp2) {
	  printf("ERROR: MAPADD requires 3 arguments!");
	  return NULL;
	}

	void* tmp3 = cdr(tmp2);
	if(!tmp3) {
	  printf("ERROR: MAPADD requires 3 arguments!");
	  return NULL;
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
	  printf("ERROR: MAPDEL requires 2 arguments!");
	  return NULL;
	}

	void* tmp2 = cdr(tmp1);
	if(!tmp2) {
	  printf("ERROR: MAPDEL requires 2 arguments!");
	  return NULL;
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
	
	void* tmp1 = car(cdr(list));
	
	// First loop over all the values and eval the values...
	// Add in the values as we go, it doesn't cost us anything...
	void* newenv = env;

	// loop over the variables (arg1)
	for(void* i=tmp1; i; i=cdr(i)) {
	  
	  void* pair = car(i);
	  
	  void* name = car(pair);
	  void* value = car(cdr(pair));
	  
	  value = eval(value, newenv); 
	  if(is_error(value)) {
	    return value;
	  }
	  
	  newenv = cons(cons(cons(name, value), car(newenv)), cdr(newenv));	  
	}
	
	void* ret = NULL;
	void* tmp2 = cdr(cdr(list));

	// loop over the code...
	for(void* i=tmp2; i!=NULL; i=cdr(i)) {

	  ret = eval(car(i), newenv);
	  if(is_error(ret)) {
	    return ret;
	  }
	}
	return ret;
      }
      break;
      
    case N_LAMBDA:
      printf("LAMBDA not yet implemented!!!\n");
      break;
      
    default:

      printf("Unknown native int function!!!\n");
      return NULL;
    }

  case TYPE_CNR:
    {
      if(!cdr(list)) {
	printf("EVAL requires ONE argument!\n");
	return NULL;
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
    
  default:

    printf("This type doesn't have a function handler (type = %s)!!!\n", return_type_c_string(o));
    break;
  }
  
  return NULL;
}

void* tread(void* env) {

  void* ret = NULL;

  void* tmp =  cassoc("*INPUT*", cdr(env)); 
  if(!tmp || !cdr(tmp)) {
    return ERROR("Could not find *INPUT* var!");
  }
  tmp = cdr(tmp);
  FILE* input = (FILE*) to_pointer(tmp)->p;    
  
  yyscan_t scanner;

  // Initialize a scanner instance for Flex
  yylex_init(&scanner);
  // Set the input file for the lexer
  yyset_in(input, scanner);
  int parseResult = yyparse(scanner, &ret);
  
  if (parseResult != 0) {
    printf("Parsing failed. (%p)\n", ret);
  }
  
  // Clean up
  yylex_destroy(scanner);
  return ret;
}

