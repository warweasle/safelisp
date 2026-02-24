#ifndef SAFELISP_H
#define SAFELISP_H

#include <ctype.h>
#include <string.h>
#include <stdio.h>

#include <gc.h>
#include <gmp.h>


#ifdef __cplusplus
extern "C" {
#endif

  
  // Override standard memory allocation functions
#define malloc(n) GC_malloc(n)
#define free(p) GC_free(p)
#define realloc(p, n) GC_realloc(p, n)

  // We can currently have 256 types. 
#define TYPE_BIT_MASK 0xFF
#define TYPE_FLAGS_BIT_MASK (~TYPE_BIT_MASK)

  // Define bit masks
#define EVENT_FLAG (1<<31)
#define RED_BLACK_FLAG (1<<30)

#define ERROR(msg)		  \
  error(create_symbol("ERROR"),					\
	create_string_type_from_string((msg), TYPE_STRING))
  
  // Set the event flag
  void set_event_flag(void* value);
  // Clear the event flag
  void clear_event_flag(void* value);
  // Check if the event flag is set
  int is_event_flag_set(void* value);
  // Set the red/black flag
  void set_red_black_flag(void* value);
  // Clear the red/black flag
  void clear_red_black_flag(void* value);
  // Check if the red/black flag is set
  int is_red_black_flag_set(void* value);

  typedef enum {
    TYPE_NULL = 0,
    TYPE_CONS = 1,
    TYPE_TRUE,
    TYPE_SYMBOL,
    TYPE_NATIVE,
    TYPE_NATIVE_INT,
    TYPE_CHAR,
    TYPE_STRING,
    TYPE_RESIZABLE_STRING,
    TYPE_INT,
    TYPE_RATIONAL,
    TYPE_FLOAT,
    TYPE_LAMBDA,
    TYPE_RAW,
    TYPE_INT8, TYPE_UINT8, TYPE_FLOAT8, TYPE_DOUBLE8, TYPE_LONG_DOUBLE8,
    TYPE_INT16, TYPE_UINT16, TYPE_FLOAT16, TYPE_DOUBLE16, TYPE_LONG_DOUBLE16,
    TYPE_INT32, TYPE_UINT32, TYPE_FLOAT32, TYPE_DOUBLE32, TYPE_LONG_DOUBLE32,
    TYPE_INT64, TYPE_UINT64, TYPE_FLOAT64, TYPE_DOUBLE64, TYPE_LONG_DOUBLE64,
    TYPE_INT128, TYPE_UINT128, TYPE_FLOAT128, TYPE_DOUBLE128, TYPE_LONG_DOUBLE128,
    TYPE_CHAR_ARRAY,
    TYPE_POINTER,
    TYPE_RB_TREE,
    TYPE_QUOTE,
    TYPE_BACKTICK,
    TYPE_COMMA,
    TYPE_SPLICE,
    TYPE_CNR,
    TYPE_ERROR,
  } ValueType;

  typedef enum {
	N_CONS,
	N_LIST,
	N_IF,
	N_TYPE,
	N_AND,
	N_OR,
	N_NOT,
	N_APPEND,
	N_ASSOC,
	N_EVAL,
	N_EQL,
	N_COND,
	N_TO_STRING,
	N_PRINT,
	N_SET,
	N_CAR,
	N_CDR,
	N_CNR,
	N_LOOP,
	N_BREAK,
	N_WHILE,
	N_READ,
	N_MAPMAKE,
	N_MAPADD,
	N_MAPGET,
	N_MAPSET,
	N_MAPDEL,
	N_LAMBDA,
	N_LET,
	N_CAT,
	N_MULT,
	N_DIV,
	N_ADD,
	N_SUB
  } nativeType;
  
#define get_type(ptr) ((ptr) ? (*(ValueType*)(ptr) & TYPE_BIT_MASK) : TYPE_NULL)
  //#define get_type(ptr) (*(ValueType*)(ptr) & TYPE_BIT_MASK)
#define get_flags(ptr) (*(ValueType*)(ptr) & TYPE_FLAGS_BIT_MASK)
#define get_getctype(ptr) ((*(ValueType*)(ptr) & SIZE_MASK)>>12)
#define is_type(ptr, ty) (get_type(ptr) == (ty))
#define is_cons(ptr) (is_type(ptr, TYPE_CONS))
#define is_error(ptr) (is_type(ptr, TYPE_ERROR))
#define is_rb_tree(ptr) (is_type(ptr, TYPE_RB_TREE))
#define is_str(ptr) (is_type(ptr, TYPE_STRING))
#define is_int(ptr) (is_type(ptr, TYPE_STRING))
#define is_float(ptr) (is_type(ptr, TYPE_STRING))
#define is_rational(ptr) (is_type(ptr, TYPE_STRING))
#define is_number(ptr) (is_int(ptr) || is_float(ptr) || is_rational(ptr))
#define car(o) (((cons_cell*)o)->car)
#define cdr(o) (((cons_cell*)o)->cdr)
#define to_cons(o) ((cons_cell*)o)
#define to_string(o) ((string_type*)o)
#define to_raw(o) ((rawtype*)o)
#define to_int(o) ((int_type*)o)
#define to_float(o) ((float_type*)o)
#define to_rational(o) ((rational_type*)o)
#define to_pointer(o) ((pointer_type*)o)
#define to_char(o) ((char_type*)o)
#define to_native(o) ((native_type*)o)
  
  typedef struct cons_cell {
    ValueType type;
    void* car;
    void* cdr;
  } cons_cell;

  typedef cons_cell *cc;

  typedef struct {
    ValueType type;
    void* func;
  } native_type;

  typedef struct {
    ValueType type;
    int size;
    char str[];
  } string_type;

  typedef struct {
    ValueType type;
    size_t pos;
    size_t len;
    char* str;
  } resizable_string_type;

  typedef struct {
	ValueType type;
	char c;
  } char_type;

  typedef struct {
	ValueType type;
	mpz_t num;
  } int_type;

  typedef struct {
	ValueType type;
	mpf_t num;
  } float_type;

  typedef struct {
	ValueType type;
	mpq_t num;
  } rational_type;

  typedef struct {
    ValueType type;
    void* p;
  } pointer_type;

  // Init and other...
  void* init_safelisp(FILE* input, FILE* output);
  
  // Cons cell functions
  cc cons(void* car, void* cdr);
  cc make_rb_tree();
  cc error(void* car, void* cdr);
  int list_length(void* list);
  int is_list(void* list);
  cc last(void* lst);

  // Bool functions
  // Why is this chartype? 
  void* create_true_type();
  void* create_false_type();
    
  // Char functions
  char_type* create_char_type(char c);

  // Quote is handled with a cons cell...
  cc create_quotetype(ValueType Type, void* car);
  
  // String_type functions
  string_type* create_string_type(size_t len, ValueType Type);
  string_type* create_string_type_and_copy(size_t len, const char* str, ValueType Type);
  string_type* create_string_type_from_string(const char* str, ValueType Type);
  int string_compare(void* a, void* b);
  int raw_string_compare(const void* a, const void* b);
  string_type* create_symbol(const char* str);
  string_type* create_symbol_and_copy(size_t len, const char* str);

    // resizable_string_type functions
  // Functions to create and manage resizable string types
  resizable_string_type* create_resizable_string_type(size_t buff_size, ValueType Type);
  resizable_string_type* create_resizable_string_type_and_copy(size_t len, const char* str, ValueType Type);
  resizable_string_type* create_resizable_string_type_from_string(const char* str, ValueType Type);
  string_type* create_string_type_from_resizable_string(resizable_string_type* resizeable);
  resizable_string_type* putch_resizable_array(resizable_string_type* arr, char c);
  resizable_string_type* putstr_resizable_array(resizable_string_type* arr, char* s);

  // native function and data functions
  char_type* create_native_int_type(nativeType type);

  // Functions to create and manage integer and float types
  int_type* create_int_type(double i);
  rational_type* create_rational_type();
  float_type* create_float_type();

  pointer_type* create_pointer_type(void* p);
  cc create_lambda(void* args, void* code);
  cc make_cnr(void* cnr);
  
  // Important functions
  int compare(void* a, void* b);
  void* equal(void* a, void* b); 
  void* eval_list(void* list, void* env);
  void* return_type(void* o); 
  void* assoc(void* item, void* list);
  void* cassoc(char* str, void* list);
  void* eval(void* list, void* env);
  void* tread(void* env);
  void* append(void* a, void* b);
  
#ifdef __cplusplus
}
#endif

#include "printer.h"
#include "rb-tree.h"

#endif // SAFELISP_H

