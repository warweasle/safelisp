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
