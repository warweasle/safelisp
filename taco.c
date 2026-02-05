#include "taco.h"


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

void* init_taco() {
  GC_INIT();
  mp_set_memory_functions(GC_malloc, gmp_gc_realloc, gmp_gc_free);

  return NULL;
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

cc cons(void* car, void* cdr) {
  cc ret = (cc) GC_malloc(sizeof(cons_cell));

  ret->type = TYPE_CONS;
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

true_type ttrue = {TYPE_TRUE};

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
  strncpy(sym->str, str, len); // Copy up to len characters
  sym->str[len] = '\0'; // Ensure null termination

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
  strncpy(sym->str, str, len); // Copy up to len characters
  sym->str[len] = '\0'; // Ensure null termination

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
  }
  
  return arr;
}

resizable_string_type* putstr_resizable_array(resizable_string_type* arr, char* s) {

  if(!is_type(arr, TYPE_RESIZABLE_STRING)) {
	
	printf("Error, not a resizeable string!\n");
	return NULL;
  }

  size_t strl = strlen(s);
  size_t totalRequired = arr->pos + strl;
						 
  if(totalRequired >= arr->len - 1) {

	// Gotta make some room...
		
	arr->len = totalRequired * totalRequired; 
	arr->str = resize_string(arr->str, arr->len);
  }

  if(arr->str != NULL) {
  
	strncpy(arr->str + arr->pos, s, strl);
	arr->pos = totalRequired;
  }
  
  return arr;
}

int string_compare(const void* a, const void* b) {
  const string_type* str1 = (const string_type*)a;
  const string_type* str2 = (const string_type*)b;

  // Ensure both are valid strings using is_str
  if (!is_str(str1) || !is_str(str2)) {
	return 0; // Consider returning 0 or handle the error as appropriate
  }

  //printf("Comparing: %s vs %s\n", str1->str, str2->str);
  
  // Compare the strings
  return strcmp(str1->str, str2->str);
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

void* equal(void* a, void* b) {

  ValueType at = get_type(a);
  ValueType bt = get_type(b);

  if(at != bt) return NULL;

  switch(at) {
  
  case TYPE_NULL:
    return create_true_type(); 
    break;
    
  case TYPE_CONS:
    {
      cc ac = to_cons(a);
      cc bc = to_cons(b);
      if(equal(ac->car, bc->car) &&
	 equal(ac->cdr, bc->cdr)) {

	return create_true_type(); 
      }
      else {
	return NULL;
      }
    }
    break;
    
  case TYPE_SYMBOL:
  case TYPE_STRING:
  
    if(string_compare(a, b) == 0) {
      return create_true_type(); 
    }
    else {
      return NULL;
    }
    break;

  case TYPE_TRUE:
  case TYPE_QUOTE:
  case TYPE_BACKTICK:
  case TYPE_COMMA:
  case TYPE_SPLICE:
  
    return create_true_type();
    break;

  case TYPE_INT:
    if(mpz_cmp(to_int(a)->num, to_int(b)->num)) {
      return create_true_type();
    }
    else {
      return NULL;
    }
    break;
    
  case TYPE_FLOAT:
    if(mpf_cmp(to_float(a)->num, to_float(b)->num)) {
      return create_true_type();
    }
    else {
      return NULL;
    }
    break;

  case TYPE_RATIONAL:

    break;

  case TYPE_NATIVE_INT:
    if(to_char(a)->c == to_char(b)->c) {
      return create_true_type();
    }
    else {
      return NULL;
    }

    break;
    
  case TYPE_CHAR:
  case TYPE_RESIZABLE_STRING:
  case TYPE_NATIVE:
  case TYPE_FUNC:
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

    return NULL;
    break;
    
  
  default:
  }
  

  return create_true_type();
}

char* return_type_c_string(void* o) {

  ValueType type = get_type(o);

  printf("return_type_c_string = %i\n", type);
  
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
  case TYPE_FUNC:
    return "FUNCTION";
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

  if(car(list) && 
     car(car(list)) && 
     equal(item, car(car(list)))) {
    return car(list);
  }

  return assoc(item, cdr(list));
}

void* eval(void* list, void* env) {

  printf("eval: TYPE: %s\n", return_type_c_string(list));

  ValueType type = get_type(list);
  
  if(type == TYPE_CONS ||
     type == TYPE_QUOTE) {
    return eval_list(list, env);
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
  
  void* o = car(to_cons(list));
  ValueType type = get_type(list);
  printf("Type = %s\n", return_type_c_string(list));
  
  switch(type) {
  case TYPE_CONS:
    printf("Function calling not yet available!!!\n");
    break;

  case TYPE_QUOTE:
    printf("QUOTE!!!\n");
    if(!car(list)) {
      printf("Error: quote requires something after it!\n");
      return NULL;
    }
    
    return car(list);
    break;
    
  case TYPE_NATIVE_INT:

    switch(to_char(o)->c) {
    case N_CONS:
      {
	void* tmp = cdr(list);
	void* a = eval(car(tmp), env);
	void* b = NULL;

	tmp = cdr(tmp);
	if(tmp) {
	  b = car(tmp);
	}

	return cons(a, b);
      }
      break;
      
    case N_LIST:
      return eval_rest(cdr(list), env);
      break;
  
    case N_IF:
      {
	void* pred = cdr(list);
	void* truth = cdr(cdr(list));
	
	if(!pred) {
	  printf("ERROR: nothing to IF!\n");
	  return NULL;
	}

	if(!truth) {
	  printf("Nothing to execute in IF statement!\n");
	  return NULL;
	}
	
	void* predicate = eval(car(pred), env);
	if(predicate) {
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
      
    case N_NULL:
      if(!cdr(list)) {
	printf("NULL requires ONE argument!\n");
	return NULL;
      }

      if(cdr(cdr(list))) {
	printf("NULL requires only ONE argument!\n");
	return NULL;
      }

      void* result = eval(car(cdr(list)), env);
      if(result) return NULL;
      else return create_true_type();
      break;
      
    case N_AND:

      {
	void* result = create_true_type();
	for(void* i; (i = cdr(list)); i = cdr(i)) {
	  result = eval(cdr(i), env);

	  if(result == NULL) return NULL;
	}

	return result;
      }
      break;

    case N_OR:
      
      {
	void* result = create_true_type();
	for(void* i; (i = cdr(list)); i = cdr(i)) {
	  result = eval(cdr(i), env);

	  if(result) return result;
	}

	return NULL;
      }
      break;
      
    case N_APPEND:
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
      
    default:

      printf("Unknown native int function!!!\n");
      return NULL;
    }

  default:

    printf("This type doesn't have a function handler!!!\n");
    break;
  }
  
  return NULL;
}
