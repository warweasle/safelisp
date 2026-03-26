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

// ENV layout is ((local local global) . internal) 
void* init_safelisp(FILE* input, FILE* output) {
  GC_INIT();
  mp_set_memory_functions(GC_malloc, gmp_gc_realloc, gmp_gc_free);

  ttrue = TYPE_TRUE;
  
  void* ret = NULL;
  ret = cons(cons(create_symbol("*INPUT*"), create_pointer_type(input, TYPE_POINTER)), ret);
  ret = cons(cons(create_symbol("*OUTPUT*"), create_pointer_type(output, TYPE_POINTER)), ret);
  
  return cons(cons(make_rb_tree(), NULL), ret);
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

  case TYPE_RATIONAL:
    {
      return mpz_cmp_ui(mpq_numref(to_rational(o)->num), 0) == 0 ? -1 : 0;
    }
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

void* append(void* a, void* b) {
  if(!a || !is_cons(a)) {
    return ERROR("APPEND requires the first argument to be a list!");
  }
  
  cc l = last(a);
  
  l->cdr = b;
  
  return a;
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

  if(!lst) return NULL;
  
  if(!is_cons(lst)) {
    printf("ERROR, You sent a non-list to LAST!!!\n");
    return NULL;
  }

  cons_cell* i;
  for(i=lst; i != NULL && get_type(i) == TYPE_CONS && cdr(i); i=cdr(i)) {/* Nothing here on purpose */}

  return i;
  
}

cc butlast(void* lst) {

  if(lst && cdr(lst)) {
    return cons(car(lst), butlast(cdr(lst)));
  }
  
  return NULL;
}

cc create_quotetype(ValueType Type, void* car) {

  cc ret = (cc) GC_malloc(sizeof(cons_cell));
  ret->type = Type;
  ret->car = car;
  ret->cdr = NULL;
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
	     
pointer_type* create_pointer_type(void* p, ValueType Type) {
  pointer_type* ret = (pointer_type*)GC_malloc(sizeof(pointer_type)); 
  if (!ret) return NULL; // Check for allocation failure
  ret->type = Type;
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

int string_compare(void* a, void* b) {
   
  // Compare the strings
  int ret = strcmp(to_string(a)->str, to_string(b)->str);
  
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

  /* printf("\nASSOC!!!\n"); */
  /* print(stdout, item, 10); */
  /* printf("\n"); */
  /* print(stdout, list, 10); */
  /* printf("\n"); */

  for(void* i=list; i; i=cdr(i)) {
    if (car(car(i))) 
      {
	if(compare(item, car(car(i))) == 0) { 
	  return car(i);	  
	}
	
      }    
  }
  
  return NULL;
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

