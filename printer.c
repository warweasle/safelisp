#include <gmp.h>
#include "printer.h"

void print(FILE* output, void* o, int base) {
  
  if(o == NULL) {
    fprintf(output, "NULL");
    fflush(output);
    return;
  }

  int t = get_type(o);
  switch(t) {

  case TYPE_TRUE:
    fprintf(output, "TRUE");
    break;
    
  case TYPE_CONS:
    {
      fprintf(output, "(");

      char first = 1;
      for(; o != NULL && is_cons(o); o=cdr(o)) {
      
	void* tmp = car(o);
	if(first) {first = 0;}
	else {fprintf(output, " ");}

	fflush(output);
	print(output, tmp, base);
	//if so that's fine move along. But if cdr != cons then dot notation.
      
	if(to_cons(o)->cdr && !is_cons(to_cons(o)->cdr)) {
	  fprintf(output, " . ");
	  fflush(output);
	  print(output, to_cons(o)->cdr, base);
	  break;
	}
      }
      fprintf(output, ")");
    }
    
    break;

    /* case TYPE_RB_TREE: */

    /* 	{ */
    /* 	  //fprintf(output, "(RB:"); */
	
    /* 	  print(output, ((rb_treetype*) car(o))->root, base); */

    /* 	//fprintf(output, ")"); */
    /* 	} */
    /* break; */
	
  case TYPE_QUOTE:
    fprintf(output, "'");
    fflush(output);
    print(output, to_cons(o)->car, base);
    break;

  case TYPE_BACKTICK:
    fprintf(output, "`");
    fflush(output);
    print(output, to_cons(o)->car, base);
    break;

  case TYPE_COMMA:
    fprintf(output, ",");
    fflush(output);
    print(output, to_cons(o)->car, base);
    break;

  case TYPE_SPLICE:
    fprintf(output, ",@");
    fflush(output);
    print(output, to_cons(o)->car, base);
    break;
		
  case TYPE_SYMBOL:
    fprintf(output, "%s", ((string_type*)o)->str);
    break;
	
  case TYPE_INT:
    mpz_out_str(output, base, to_int(o)->num);
    break;

  case TYPE_NATIVE_INT:
    // Placeholder until we get a real native print list.
    {
      char_type* n = to_char(o);

      switch(n->c) {

      case N_CONS:
	fprintf(output, "CONS");
	break;

      case N_LIST:
	fprintf(output, "LIST");
	break;

      case N_IF:
	fprintf(output, "?");
	break;

      case N_TYPE:
	fprintf(output, "TYPE");
	break;
		
      case N_NOT:
	fprintf(output, "!");
	break;
		
      case N_AND:
	fprintf(output, "&&");
	break;
		
      case N_OR:
	fprintf(output, "||");
	break;
		
      case N_APPEND:
	fprintf(output, "APPEND");
	break;
		
      case N_ASSOC:
	fprintf(output, "ASSOC");
	break;
		
      case N_EVAL:
	fprintf(output, "EVAL");
	break;

      case N_PRINT:
	fprintf(output, "PRINT");
	break;

      case N_SET:
	fprintf(output, "=");
	break;

      case N_CAR:
	fprintf(output, "CAR");
	break;

      case N_CDR:
	fprintf(output, "CDR");
	break;

      case N_TO_STRING:
	fprintf(output, "TO-STRING");
	break;

      case N_BREAK:
	fprintf(output, "BREAK");
	break;
	
      case N_LOOP:
	fprintf(output, "LOOP");
	break;
      
      case N_WHILE:
	fprintf(output, "WHILE");
	break;

      case N_LET:
	fprintf(output, "LET");
	break;

      case N_LAMBDA:
	fprintf(output, "LAMBDA");
	break;

      case N_ADD:
	fprintf(output, "+");
	break;
	
      case N_SUB:
	fprintf(output, "-");
	break;
	
      case N_MULT:
	fprintf(output, "*");
	break;
	
      case N_DIV:
	fprintf(output, "/");
	break;      
	
      default:

	fprintf(output, "UNKNOW_NATIVE");
      }
    }
    break;
	
  case TYPE_FLOAT:
    mpf_out_str(output, base, 0, to_float(o)->num);
    break;

  case TYPE_RATIONAL:
    mpq_out_str(output, base, to_rational(o)->num);
    break;
    
  case TYPE_STRING:
    fprintf(output, "\"%s\"", to_string(o)->str);
    break;

    /* case TYPE_CHAR: */
    /*   fprintf(output, "#\\%c", to_string(o)->str[0]); */
    /* 	break; */

  case TYPE_POINTER:
    fprintf(output, "<POINTER:%p>", to_pointer(o)->p);
    break;

  case TYPE_RB_TREE:
    fprintf(output, "<MAP:%p>", o);
    break;

  case TYPE_CNR:
    fprintf(output, "C%sR", to_string(car(o))->str);
    break;
    
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

    printf("Specialized array type data isn't printable yet, defaulting to hex.\n");
    /* case TYPE_RAW: */
    /* 	fprintf(output, "#x"); */
    /* 	{ */
    /* 	  rawtype* raw = to_raw(o); */

    /*     for (size_t i = 0; i < raw->size; ++i) { */
    /* 		fprintf(output, "%02x", (unsigned char)raw->data[i]); */
    /*     } */
    /*   } */
    /*   break; */

  case TYPE_ERROR:
    fprintf(output, "<ERROR: ");
    print(output, to_cons(o)->cdr, base);
    fprintf(output, ">");
    break;

	
  default:
    printf("We have no idea what %p is.\n", o);
  }

  fflush(output);
}

