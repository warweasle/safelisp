#include "taco.h"

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
