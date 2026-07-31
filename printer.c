#include "printer.h"
#include <gmp.h>

// Every stringify_* function appends its type's representation onto buf,
// which is backed by GC memory (see putch_resizable_array/putstr_resizable_array).
// No libc heap (malloc/open_memstream/free) is used anywhere in this file.

static void stringify_dispatch(resizable_string_type* buf, void* o, int base);

static void stringify_null(resizable_string_type* buf) {
  putstr_resizable_array(buf, "NULL");
}

static void stringify_true(resizable_string_type* buf) {
  putstr_resizable_array(buf, "TRUE");
}

static void stringify_symbol(resizable_string_type* buf, void* o) {
  putstr_resizable_array(buf, ((string_type*)o)->str);
}

static void stringify_string(resizable_string_type* buf, void* o) {
  putch_resizable_array(buf, '"');
  putstr_resizable_array(buf, to_string(o)->str);
  putch_resizable_array(buf, '"');
}

static void stringify_char(resizable_string_type* buf, void* o) {
  putch_resizable_array(buf, to_char(o)->c);
}

static void stringify_int(resizable_string_type* buf, void* o, int base) {
  char* digits = mpz_get_str(NULL, base, to_int(o)->num);
  putstr_resizable_array(buf, digits);
}

static void stringify_rational(resizable_string_type* buf, void* o, int base) {
  char* digits = mpq_get_str(NULL, base, to_rational(o)->num);
  putstr_resizable_array(buf, digits);
}

// mpf_get_str returns bare significant digits (an implicit radix point to
// the left of the first digit) plus an exponent, rather than a formatted
// string. Reassemble those into plain decimal notation, e.g. "3.14159".
static void stringify_float(resizable_string_type* buf, void* o, int base) {
  mp_exp_t exp;
  char* digits = mpf_get_str(NULL, &exp, base, 0, to_float(o)->num);

  int neg = (digits[0] == '-');
  char* d = digits + neg;
  size_t ndigits = strlen(d);

  if(neg) putch_resizable_array(buf, '-');

  if(ndigits == 0) {
    // op was zero.
    putstr_resizable_array(buf, "0.0");
    return;
  }

  if(exp <= 0) {
    putstr_resizable_array(buf, "0.");
    for(mp_exp_t i = 0; i < -exp; i++) putch_resizable_array(buf, '0');
    putstr_resizable_array(buf, d);
  }
  else if((size_t)exp >= ndigits) {
    putstr_resizable_array(buf, d);
    for(size_t i = ndigits; i < (size_t)exp; i++) putch_resizable_array(buf, '0');
    putstr_resizable_array(buf, ".0");
  }
  else {
    char saved = d[exp];
    d[exp] = '\0';
    putstr_resizable_array(buf, d);
    putch_resizable_array(buf, '.');
    d[exp] = saved;
    putstr_resizable_array(buf, d + exp);
  }
}

static const char* native_int_name(char c) {
  switch(c) {
  case N_CONS:      return "CONS";
  case N_LIST:      return "LIST";
  case N_IF:        return "?";
  case N_TYPE:      return "TYPE";
  case N_NOT:       return "!";
  case N_AND:       return "&&";
  case N_OR:        return "||";
  case N_APPEND:    return "APPEND";
  case N_ASSOC:     return "ASSOC";
  case N_EVAL:      return "EVAL";
  case N_PRINT:     return "PRINT";
  case N_SET:       return "=";
  case N_CAR:       return "CAR";
  case N_CDR:       return "CDR";
  case N_TO_STRING: return "TO-STRING";
  case N_BREAK:     return "BREAK";
  case N_LOOP:      return "LOOP";
  case N_WHILE:     return "WHILE";
  case N_LET:       return "LET";
  case N_LAMBDA:    return "lambda";
  case N_ADD:       return "+";
  case N_SUB:       return "-";
  case N_MULT:      return "*";
  case N_DIV:       return "/";
  case N_PROGN:     return "...";
  case N_PROG1:     return "1...";
  case N_MAP:       return "MAP";
  case N_REDUCE:    return "REDUCE";
  case N_FILTER:    return "FILTER";
  default:          return "UNKNOW_NATIVE";
  }
}

static void stringify_native_int(resizable_string_type* buf, void* o) {
  putstr_resizable_array(buf, native_int_name(to_char(o)->c));
}

static void stringify_pointer(resizable_string_type* buf, void* o) {
  char tmp[32];
  snprintf(tmp, sizeof(tmp), "<POINTER:%p>", to_pointer(o)->p);
  putstr_resizable_array(buf, tmp);
}

// QUOTE/BACKTICK/COMMA/SPLICE all share the same shape: a marker followed
// by the recursively stringified inner form.
static void stringify_marked_form(resizable_string_type* buf, void* o, int base, const char* marker) {
  putstr_resizable_array(buf, marker);
  stringify_dispatch(buf, to_cons(o)->car, base);
}

static void stringify_cons(resizable_string_type* buf, void* o, int base) {
  putch_resizable_array(buf, '(');

  char first = 1;
  for(; o != NULL && is_cons(o); o = cdr(o)) {

    void* tmp = car(o);
    if(first) { first = 0; }
    else { putch_resizable_array(buf, ' '); }

    stringify_dispatch(buf, tmp, base);

    if(to_cons(o)->cdr && !is_cons(to_cons(o)->cdr)) {
      putstr_resizable_array(buf, " . ");
      stringify_dispatch(buf, to_cons(o)->cdr, base);
      break;
    }
  }
  putch_resizable_array(buf, ')');
}

// A tree NODE is (key . (left . (right . parent))) -- printing it via the
// generic cons walker would eventually follow a non-root node's parent
// pointer back up the tree and loop forever. Walk key/left/right only.
static void stringify_rb_node(resizable_string_type* buf, void* node, int base) {
  if(!node) return;

  void* left = RB_LEFT(node);
  void* right = RB_RIGHT(node);

  if(left) {
    stringify_rb_node(buf, left, base);
    putch_resizable_array(buf, ' ');
  }

  stringify_dispatch(buf, RB_GET_KEY(node), base);

  if(right) {
    putch_resizable_array(buf, ' ');
    stringify_rb_node(buf, right, base);
  }
}

static void stringify_rb_tree(resizable_string_type* buf, void* o, int base) {
  putstr_resizable_array(buf, "(MAPMAKE");
  if(car(o)) {
    putch_resizable_array(buf, ' ');
    stringify_rb_node(buf, car(o), base);
  }
  putch_resizable_array(buf, ')');
}

static void stringify_cnr(resizable_string_type* buf, void* o) {
  putch_resizable_array(buf, 'C');
  putstr_resizable_array(buf, to_string(car(o))->str);
  putch_resizable_array(buf, 'R');
}

static void stringify_error(resizable_string_type* buf, void* o, int base) {
  putstr_resizable_array(buf, "<ERROR: ");
  stringify_dispatch(buf, to_cons(o)->cdr, base);
  putch_resizable_array(buf, '>');
}

static void stringify_lambda(resizable_string_type* buf, void* o, int base) {
  putstr_resizable_array(buf, "(LAMBDA ");
  stringify_dispatch(buf, car(cdr(o)), base);
  putch_resizable_array(buf, ' ');
  stringify_dispatch(buf, cdr(cdr(o)), base);
  putch_resizable_array(buf, ')');
}

static void stringify_unprintable_array(resizable_string_type* buf) {
  putstr_resizable_array(buf, "Specialized array type data isn't printable yet, defaulting to hex.");
}

static void stringify_unknown(resizable_string_type* buf, void* o) {
  char tmp[48];
  snprintf(tmp, sizeof(tmp), "We have no idea what %p is.", o);
  putstr_resizable_array(buf, tmp);
}

static void stringify_dispatch(resizable_string_type* buf, void* o, int base) {

  if(o == NULL) { stringify_null(buf); return; }

  switch(get_type(o)) {

  case TYPE_TRUE:      stringify_true(buf); break;
  case TYPE_CONS:      stringify_cons(buf, o, base); break;
  case TYPE_QUOTE:     stringify_marked_form(buf, o, base, "'"); break;
  case TYPE_BACKTICK:  stringify_marked_form(buf, o, base, "`"); break;
  case TYPE_COMMA:     stringify_marked_form(buf, o, base, ","); break;
  case TYPE_SPLICE:    stringify_marked_form(buf, o, base, ",@"); break;
  case TYPE_SYMBOL:    stringify_symbol(buf, o); break;
  case TYPE_INT:       stringify_int(buf, o, base); break;
  case TYPE_NATIVE_INT: stringify_native_int(buf, o); break;
  case TYPE_FLOAT:     stringify_float(buf, o, base); break;
  case TYPE_RATIONAL:  stringify_rational(buf, o, base); break;
  case TYPE_STRING:    stringify_string(buf, o); break;
  case TYPE_CHAR:      stringify_char(buf, o); break;
  case TYPE_POINTER:   stringify_pointer(buf, o); break;
  case TYPE_RB_TREE:   stringify_rb_tree(buf, o, base); break;
  case TYPE_CNR:       stringify_cnr(buf, o); break;
  case TYPE_ERROR:     stringify_error(buf, o, base); break;
  case TYPE_LAMBDA:    stringify_lambda(buf, o, base); break;

  case TYPE_INT8: case TYPE_UINT8: case TYPE_FLOAT8: case TYPE_DOUBLE8: case TYPE_LONG_DOUBLE8:
  case TYPE_INT16: case TYPE_UINT16: case TYPE_FLOAT16: case TYPE_DOUBLE16: case TYPE_LONG_DOUBLE16:
  case TYPE_INT32: case TYPE_UINT32: case TYPE_FLOAT32: case TYPE_DOUBLE32: case TYPE_LONG_DOUBLE32:
  case TYPE_INT64: case TYPE_UINT64: case TYPE_FLOAT64: case TYPE_DOUBLE64: case TYPE_LONG_DOUBLE64:
  case TYPE_INT128: case TYPE_UINT128: case TYPE_FLOAT128: case TYPE_DOUBLE128: case TYPE_LONG_DOUBLE128:
  case TYPE_CHAR_ARRAY:
    stringify_unprintable_array(buf);
    break;

  default:
    stringify_unknown(buf, o);
  }
}

// Public: append o's printed representation onto buf (creating buf if NULL).
// Shared by both PRINT and TO-STRING so there is exactly one formatting
// implementation per type.
resizable_string_type* stringify(resizable_string_type* buf, void* o, int base) {
  if(!buf) buf = create_resizable_string_type(64, TYPE_RESIZABLE_STRING);
  stringify_dispatch(buf, o, base);
  return buf;
}

string_type* to_string_type(void* o, int base) {
  // TO-STRING on a string returns its raw content, unlike PRINT which
  // wraps it in quotes as write-syntax.
  if(o && is_type(o, TYPE_STRING)) return to_string(o);

  resizable_string_type* buf = stringify(NULL, o, base);
  return create_string_type_from_resizable_string(buf);
}

void print(FILE* output, void* o, int base) {
  resizable_string_type* buf = stringify(NULL, o, base);
  fwrite(buf->str, 1, buf->pos, output);
  fflush(output);
}
