#include "prefix.h"
#define EXTERN extern
#include "header.h"

/*
   information of instructions
 */
static InsnInfo insn_info_table[] = {
#include "instructions-table.h"
};

char *insn_nemonic(int opcode) {
  return insn_info_table[opcode].insn_name;
}

/*
   instruction table
 */

#define LOADBUFLEN 1024

extern uint32_t decode_escape_char(char *);
extern int print_function_table(FunctionTable *, int);
extern void print_bytecode(Instruction *, int);

FILE *file_pointer;

/*
   reads the next line from the input stream
 */
inline void step_load_code(char *buf, int buflen) {
  fgets(buf, buflen, file_pointer == NULL? stdin: file_pointer);
}

#define DELIM " \n\r"
#define DELIM2 "\n\r"
#define first_token(b) strtok(b, DELIM)
#define next_token()   strtok(NULL, DELIM)
#define next_token2()  strtok(NULL, DELIM2)

inline int check_read_token(char *buf, char *tok) {
  char *p;
  p = first_token(buf);
  if (strcmp(p, tok) != 0)
    LOG_EXIT("Error: %s is not defined", tok);
  return atoi(next_token());
}

/*
   codeloader
 */
int code_loader(Context *ctx, FunctionTable *ftable) {
  ConstantCell ctable;
  Bytecode* bytecodes;
  char buf[LOADBUFLEN];
  int nfuncs, callentry, sendentry, nlocals, ninsns;
  int i, j, ret;

  // checks the funclength and obtain the number of functions
  step_load_code(buf, LOADBUFLEN);
  nfuncs = check_read_token(buf, "funcLength");

  // reads each function
  for (i = 0; i < nfuncs; i++) {
    // callentry
    step_load_code(buf, LOADBUFLEN);
    callentry = check_read_token(buf, "callentry");

    // sendentry
    step_load_code(buf, LOADBUFLEN);
    sendentry = check_read_token(buf, "sendentry");

    // numberOfLocals
    step_load_code(buf, LOADBUFLEN);
    nlocals = check_read_token(buf, "numberOfLocals");

    // numberOfInstruction
    step_load_code(buf, LOADBUFLEN);
    ninsns = check_read_token(buf, "numberOfInstruction");

    // initializes constant table
    init_constant_cell(&ctable);

    // initilaizes bytecode array
    bytecodes = (Bytecode*)malloc(sizeof(Bytecode) * ninsns);
    if (bytecodes == NULL)
      LOG_EXIT("%dth func: cannnot malloc bytecode", i);

    // loads instructions for each function
    for (j = 0; j < ninsns; j++)
      ret = insn_load(ctx, &ctable, bytecodes, j);

    ret = update_function_table(ftable, i, &ctable, bytecodes,
                               callentry, sendentry, nlocals, ninsns);
    end_constant_cell(&ctable);
  }
  // number_functions = i;
  if (ftable_flag == TRUE)
    print_function_table(ftable, i);
  return i;
}

/*
   initializes the code loader
 */
void init_code_loader(FILE *fp) {
  file_pointer = fp;
}

/*
   finalizes the code loader
 */
void end_code_loader() {
  if (file_pointer != NULL)
    fclose(file_pointer);
}

#define NOT_OPCODE ((Opcode)(-1))

Opcode find_insn(char* s) {
  int i;
  int numinsts = sizeof(insn_info_table) / sizeof(InsnInfo);
  for (i = 0; i < numinsts; i++)
    if (strcmp(insn_info_table[i].insn_name, s) == 0)
      return i;
  // not found in the instruction table
  return NOT_OPCODE;
}

#define LOAD_OK     0
#define LOAD_FAIL  (-1)

/*
   loads an instruction
 */
int insn_load(Context *ctx, ConstantCell *constant, Bytecode *bytecodes, int pc) {
  char buf[LOADBUFLEN];
  char *tokp;
  Opcode oc;

  step_load_code(buf, LOADBUFLEN);
  tokp = first_token(buf);
  oc = find_insn(tokp);
  if (oc == NOT_OPCODE) {
    // instruction is not found in the instruction info table
    LOG_ERR("Illegal instruction: %s", tokp);
    bytecodes[pc] = (Bytecode)(-1);
    return LOAD_FAIL;
  }
  switch (insn_info_table[oc].operand_type) {
  case SMALLPRIMITIVE:
    {
      Register dst;
      dst = atoi(next_token());
      switch (oc) {
      case FIXNUM:
        bytecodes[pc] = makecode_fixnum(dst, atoi(next_token()));
        break;
      case SPECCONST:
        bytecodes[pc] =
          makecode_specconst(dst, specstr_to_jsvalue(next_token()));
        break;
      default:
        return LOAD_FAIL;
      }
      return LOAD_OK;
    }

  case BIGPRIMITIVE:
    {
      Register dst;
      dst = atoi(next_token());
      switch (oc) {
      case NUMBER:
        {
          double number;
          int index;
          number = atof(next_token());
          // writes the number into the constant table
          index = add_constant_number(ctx, constant, number);
          bytecodes[pc] = makecode_number(dst, index);
        }
        break;
      case STRING:
      case ERROR:
        {
          char *str;
          int index;
          uint32_t len;
          str = next_token2();
          // str = next_token();
          if (str == NULL) str = "";
          else len = decode_escape_char(str);
          index = add_constant_string(ctx, constant, str);
          if (oc == STRING)
            bytecodes[pc] = makecode_string(dst, index);
          else
            bytecodes[pc] = makecode_error(dst, index);
        }
        break;
#ifdef USE_REGEXP
#ifdef need_regexp
      case REGEXP:
        {
          char *str;
          int flag, index;
          uint32_t len;
          flag = atoi(next_token());
          str = next_token();
          if (str == NULL) str = "";
          else len = decode_escape_char(str);
          index = add_constant_regexp(ctx, constant, str, flag);
          bytecodes[pc] = makecode_regexp(dst, index);
        }
        break;
#endif /* need_regexp */
#endif
      default:
        return LOAD_FAIL;
      }
      return LOAD_OK;
    }

  case THREEOP:
    {
      Register op1, op2, op3;
      op1 = atoi(next_token());
      op2 = atoi(next_token());
      op3 = atoi(next_token());
      bytecodes[pc] = makecode_three_operands(oc, op1, op2, op3);
      return LOAD_OK;
    }

  case TWOOP:
    {
      Register op1, op2;
      op1 = atoi(next_token());
      op2 = atoi(next_token());
      bytecodes[pc] = makecode_two_operands(oc, op1, op2);
      return LOAD_OK;
    }

  case ONEOP:
    {
      Register op;
      op = atoi(next_token());
      bytecodes[pc] = makecode_one_operand(oc, op);
      return LOAD_OK;
    }

  case ZEROOP:
    {
      bytecodes[pc] = makecode_no_operand(oc);
      return LOAD_OK;
    }

  case UNCONDJUMP:
    {
      Displacement disp;
      disp = (Displacement)atoi(next_token());
      bytecodes[pc] = makecode_jump(oc, disp);
      return LOAD_OK;
    }

  case CONDJUMP:
    {
      Displacement disp;
      Register src;
      src = atoi(next_token());
      disp = (Displacement)atoi(next_token());
      bytecodes[pc] = makecode_cond_jump(oc, src, disp);
      return LOAD_OK;
    }

  case GETVAR:
    {
      Subscript link, offset;
      Register reg;
      link = atoi(next_token());
      offset = atoi(next_token());
      reg = atoi(next_token());
      bytecodes[pc] = makecode_getvar(oc, link, offset, reg);
      return LOAD_OK;
    }

  case SETVAR:
    {
      Subscript link, offset;
      Register reg;
      link = atoi(next_token());
      offset = atoi(next_token());
      reg = atoi(next_token());
      bytecodes[pc] = makecode_setvar(oc, link, offset, reg);
      return LOAD_OK;
    }

  case MAKECLOSUREOP:
    {
      Register dst;
      uint16_t index;
      dst = atoi(next_token());
      index = (uint16_t)atoi(next_token());
      bytecodes[pc] = makecode_makeclosure(oc, dst, index);
      return LOAD_OK;
    }

  case CALLOP:
    {
      Register closure;
      uint16_t argc;
      closure = atoi(next_token());
      argc = atoi(next_token());
      bytecodes[pc] = makecode_call(oc, closure, argc);
      return LOAD_OK;
    }

  default:
    {
      LOG_EXIT("Illegal instruction: %s\n", tokp);
      return LOAD_FAIL;
    }
  }
}

/*
   initilizes the contant table
 */ 
void init_constant_cell(ConstantCell *constant)
{
  constant->n_constant_values = 0;
  constant->constant_values =
    (JSValue*)malloc(sizeof(JSValue) * (CONSTANT_LIMIT));
}

/*
   finlaizes the constant table
 */
void end_constant_cell(ConstantCell *constant) {
  // do nothing
}

/*
   converts a special JS string (for a constant) into a JSValue
 */
JSValue specstr_to_jsvalue(const char *str) {
  if (strcmp(str, "true") == 0)
    return JS_TRUE;
  else if (strcmp(str, "false") == 0)
    return JS_FALSE;
  else if (strcmp(str, "null") == 0)
    return JS_NULL;
  else if (strcmp(str, "undefined") == 0)
    return JS_UNDEFINED;
  else
    // undefined name
    LOG_EXIT("%s is an undefined symbol.", str);
}

char *jsvalue_to_specstr(JSValue v) {
  char *s;
  if (v == JS_TRUE) s = "true";
  else if (v == JS_FALSE) s = "false";
  else if (v == JS_NULL) s = "null";
  else if (v == JS_UNDEFINED) s = "undefined";
  else s = "unknown_specstr";
  return s;
}

/*
   adds a number into the constant table.

   Currently, the same constant numbers might be added to the table
   more than once.  It might be better to avoid such duplication.
 */
int add_constant_number(Context *ctx, ConstantCell *constant, double x) {
  int index;

  index = constant->n_constant_values++;
  (constant->constant_values)[index] = double_to_number(x);
  return index;
}

/*
   adds a string into the constant table.
 */
int add_constant_string(Context *ctx, ConstantCell *constant, char *str) {
  JSValue a;
  int index;

  index = constant->n_constant_values++;
  a = cstr_to_string(NULL, str);
  // printf("updateConstantString: str = %s, a = %lld (%s)\n",
  //        str, a, stringToCStr(a));
  (constant->constant_values)[index] = a;
  return index;
}

#ifdef USE_REGEXP
#ifdef need_regexp
/*
   adds a regexp into the constant table.
 */
int add_constant_regexp(Context *ctx, ConstantCell *constant, char *pat, int flag) {
  JSValue re;
  int index;

  index = constant->n_constant_values++;
  // re = new_regexp();
  if ((re = new_normal_regexp(ctx, pat, flag)) != JS_UNDEFINED) {
    (constant->constant_values)[index] = re;
    return index;
  } else {
    LOG_ERR("an error occured in making regex.");
    return -1;
  }
}
#endif /* need_regexp */
#endif

int update_function_table(FunctionTable *ftable, int index,
                          ConstantCell *constant, Bytecode *bytecodes,
                          int callentry, int sendentry,
                          int nlocals, int ninsns) {
  int i;
  Opcode oc;
  Instruction *insns;
  void **insnptr;
  int bodysize;

#ifdef J5MODE
  if (index == 0) {
    ninsns++;
  }
#endif

  bodysize = ninsns + constant->n_constant_values;
  insns = (Instruction *)malloc(sizeof(Instruction) * bodysize);
  // The above shold be the following???
  // insns = (Instruction *)malloc(sizeof(Instruction) * ninsns +
  //                            sizeof(JSvalue) * constant->n_constant_values);
  insnptr = (void **)malloc(sizeof(void *) * ninsns);
  int loopnum = ninsns;

#ifdef J5MODE
  loopnum += (index == 0)? -1: 0;
#endif

  // rewrites the operand of STRING/NUMBER/ERROR/REGEXP instraction
  // as a relative displacement
  for (i = 0; i < loopnum; i++) {
    insnptr[i] = NULL;
    oc = get_opcode(bytecodes[i]);
    if (oc == STRING || oc == NUMBER || oc == ERROR
#ifdef USE_REGEXP
#ifdef need_regexp
        || oc == REGEXP
#endif /* need_regexp */
#endif
        ) {
      Subscript ss;
      Displacement disp;
      ss = get_big_subscr(bytecodes[i]);
      disp = calc_displacement(ninsns, i, ss);
      bytecodes[i] = update_displacement(bytecodes[i], disp);
    }
  }

  // fills the insns array
  for (i = 0; i < loopnum; i++) {
    insns[i].code = bytecodes[i];
#ifdef USE_THRESHOLD
    insns[i].hitCount = 0;
    insns[i].missCount = 0;
#endif
  }

  // writes the constants
  for (; i < bodysize; i++) {
    insns[i].code = constant->constant_values[i - ninsns];
  }

  ftable[index].insn_ptr_created = false;
  ftable[index].insns = insns;
  ftable[index].insn_ptr = insnptr;
  ftable[index].call_entry = callentry;
  ftable[index].send_entry = sendentry;
  ftable[index].n_locals = nlocals;
  ftable[index].body_size = bodysize;
  ftable[index].n_insns = ninsns;

  return 0;
}

int print_function_table(FunctionTable *ftable, int nfuncs) {
  int i, j;

  printf("number of functions = %d\n", nfuncs);
  for (i = 0; i < nfuncs; i++) {
    printf("function #%d\n", i);
    printf("call_entry: %d\n", ftable[i].call_entry);
    printf("send_entry: %d\n", ftable[i].send_entry);
    printf("n_locals: %d\n" ,ftable[i].n_locals);
    printf("n_insns: %d\n", ftable[i].n_insns);
    printf("body_size: %d\n", ftable[i].body_size);
    for (j = 0; j < ftable[i].n_insns; j++) {
      printf("%03d: %016lx --- ", j, ftable[i].insns[j].code);
      print_bytecode(ftable[i].insns, j);
    }
    for (; j < ftable[i].body_size; j++) {
      JSValue o;
      o = ftable[i].insns[j].code;
      printf("%03d: %016lx --- ", j, o);
      if (is_flonum(o))
        printf("FLONUM %lf\n", flonum_value(o));
      else if (is_string(o))
        printf("STRING \"%s\"\n", string_value(o));
#ifdef USE_REGEXP
#ifdef need_regexp
      else if (is_regexp(o))
        printf("REGEXP \"%s\"\n", regexp_pattern(o));
#endif /* need_regexp */
#endif
      else
        printf("Unexpected JSValue\n");
    }
  }
  return 0;
}

/*
   prints a bytecode instruction
 */
void print_bytecode(Instruction *insns, int j) {
  Bytecode code;
  Opcode oc;
  OperandType t;

  code = insns[j].code;
  oc = get_opcode(code);
  t = insn_info_table[oc].operand_type;
  printf("%s ", insn_info_table[oc].insn_name);
  switch (t) {
  case SMALLPRIMITIVE:
    {
      Register dst;
      int imm;
      dst = get_first_operand_reg(code);
      switch (oc) {
      case FIXNUM:
        imm = get_small_immediate(code);
        printf("%d %d", dst, imm);
        break;
      case SPECCONST:
        imm = get_small_immediate(code);
        printf("%d %s", dst, jsvalue_to_specstr(imm));
        break;
      default:
        printf("???");
        break;
      }
    }
    break;
  case BIGPRIMITIVE:
    {
      Register dst;
      Displacement disp;
      JSValue o;
      dst = get_first_operand_reg(code);
      disp = get_big_disp(code);
      o = (JSValue)(insns[j + disp].code);
      // printf("j = %d, disp = %d, o = %p\n", j, disp, (char *)o);
      switch (oc) {
      case NUMBER:
        if (is_flonum(o))
          printf("%d %f", dst, flonum_value(o));
        else
          printf("Object type mismatched: tag = %d", get_tag(o));
        break;
      case STRING:
      case ERROR:
        if (is_string(o))
          printf("%d \"%s\"", dst, string_value(o));
        else
          printf("Object type mismatched: tag = %d", get_tag(o));
        break;
#ifdef USE_REGEXP
#ifdef need_regexp
      case REGEXP:
        if (is_regexp(o))
          printf("%d %d \"%s\"", dst, regexp_flag(o), regexp_pattern(o));
        else
          printf("Object type mismatched: tag = %d", get_tag(o));
        break;
#endif /* need_regexp */
#endif // USE_REGEXP
      default:
        printf("???");
        break;
      }
    }
    break;
  case THREEOP:
    {
      Register dst, r1, r2;
      dst = get_first_operand_reg(code);
      r1 = get_second_operand_reg(code);
      r2 = get_third_operand_reg(code);
      printf("%d %d %d", dst, r1, r2);
    }
    break;
  case TWOOP:
    {
      Register dst, r1;
      dst = get_first_operand_reg(code);
      r1 = get_second_operand_reg(code);
      printf("%d %d", dst, r1);
    }
    break;
  case ONEOP:
    {
      Register dst;
      dst = get_first_operand_reg(code);
      printf("%d", dst);
    }
    break;
  case ZEROOP:
    break;
  case UNCONDJUMP:
  case TRYOP:
    {
      Displacement disp;
      disp = get_first_operand_disp(code);
      printf("%d", j + disp);
    }
    break;
  case CONDJUMP:          
    {
      Register r;
      Displacement disp;
      r = get_first_operand_reg(code);
      disp = get_second_operand_disp(code);
      printf("%d %d", r, j + disp);
    }
    break;
  case GETVAR:
    {
      Register dst;
      Subscript link, ss;
      link = get_first_operand_subscr(code);
      ss = get_second_operand_subscr(code);
      dst = get_third_operand_reg(code);
      printf("%d %d %d", link, ss, dst);
    }
    break;
  case SETVAR:
    {
      Register src;
      Subscript link, ss;
      link = get_first_operand_subscr(code);
      ss = get_second_operand_subscr(code);
      src = get_third_operand_reg(code);
      printf("%d %d %d", link, ss, src);
    }
    break;
  case MAKECLOSUREOP:
    {
      Register dst;
      Subscript ss;
      dst = get_first_operand_reg(code);
      ss = get_second_operand_subscr(code);
      printf("%d %d", dst, ss);
    }
    break;
  case CALLOP:
    {
      Register f;
      int na;
      f = get_first_operand_reg(code);
      na = get_second_operand_int(code);
      printf("%d %d", f, na);
    }
    break;
  case UNKNOWNOP:
    break;
  }
  putchar('\n');
}

uint32_t decode_escape_char(char *str) {
  char *src, *dst;
  int dq;
  char c;

  src = dst = str;
  dq = 0;
  if ((c = *src++) == '\"') {
    dq = 1;
    c = *src++;
  }
  while (1) {
    if (dq == 1 && c == '\"') break;
    if (dq == 0 && c == '\0') break;
    if (c != '\\') {
      *dst++ = c;
      c = *src++;
      continue;
    }
    switch (c = *src++) {
    case '0': *dst++ = '\0'; break;
    case 'a': *dst++ = '\a'; break;
    case 'b': *dst++ = '\b'; break;
    case 'f': *dst++ = '\f'; break;
    case 'n': *dst++ = '\n'; break;
    case 'r': *dst++ = '\r'; break;
    case 't': *dst++ = '\t'; break;
    case 'v': *dst++ = '\v'; break;
    case '\\': *dst++ = '\\'; break;
    case '\'': *dst++ = '\''; break;
    case '\"': *dst++ = '\"'; break;
    case 'x':
      {
        int k = 0, i;
        for (i = 0; i <= 1; i++) {
          c = *src++;
          if (c == ' ') c = '0';
          k <<= 4;
          if ('0' <= c && c <= '9') k += c - '0';
          else if ('a' <= c && c <= 'f') k += c + 10 - 'a';
          else if ('A' <= c && c <= 'F') k += c + 10 - 'A';
        }
        *dst++ = (char)k;
      }
      break;
    default: *dst++ = c; break;
    }
    c = *src++;
  }
  *dst = '\0';
  return (uint32_t)(dst - str);
}
