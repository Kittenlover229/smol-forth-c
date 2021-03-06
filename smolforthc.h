#ifndef __KITTENLOVER__SMOLFORTH_C__
#define __KITTENLOVER__SMOLFORTH_C__

/*

extensions:
SMOLFORTH_USE_EXT_DEFAULT_BUILTINS_LIST - default functions like drop, swap, dup

*/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef uint16_t smolforth_status_ret;

#define SMOLFORTH_STATUS_OK ((smolforth_status_ret)0x0000)
#define SMOLFORTH_STATUS_STACK_UNDERFLOW ((smolforth_status_ret)0x0010)
#define SMOLFORTH_STATUS_TYPE_ERROR ((smolforth_status_ret)0x0020)
#define SMOLFORTH_STATUS_UNDEFINED_NAME ((smolforth_status_ret)0x0040)

#define SMOLFORTH_MAX_WORD_LENGTH 16
#define SMOLFORTH_MAX_WORD_COUNT_IN_LIST 32

typedef enum smolforth_tok_kind {
  SMOLFORTH_TOK_INTEGER,
  SMOLFORTH_TOK_WORD,
  SMOLFORTH_TOK_DOUBLE,
} smolforth_tok_kind;

typedef struct smolforth_tok {
  smolforth_tok_kind kind;
  union {
    long long int as_integer;
    const char *as_word;
    double as_double;
  };
} smolforth_tok;

smolforth_tok smolforth_new_tok_integer(long long int integer) {
  smolforth_tok ret;
  ret.kind = SMOLFORTH_TOK_INTEGER;
  ret.as_integer = integer;
  return ret;
}

smolforth_tok smolforth_new_tok_word(const char *word) {
  smolforth_tok ret;
  ret.kind = SMOLFORTH_TOK_WORD;
  ret.as_word = word;
  return ret;
}

smolforth_tok smolforth_new_tok_double(double doublee) {
  smolforth_tok ret;
  ret.kind = SMOLFORTH_TOK_DOUBLE;
  ret.as_double = doublee;
  return ret;
}

typedef enum smolforth_unit_kind {
  SMOLFORTH_UNIT_INTEGER,
  SMOLFORTH_UNIT_DOUBLE,
} smolforth_unit_kind;

typedef struct smolforth_unit {
  smolforth_unit_kind kind;
  union {
    long long int as_integer;
    double as_double;
  };
} smolforth_unit;

typedef struct smolforth_unit_stack {
  smolforth_unit *units;
  size_t len;
  size_t maxlen;
} smolforth_unit_stack;

smolforth_unit_stack smolforth_unit_stack_new(smolforth_unit *units,
                                              size_t len) {
  smolforth_unit_stack ret;
  ret.len = 0;
  ret.maxlen = len;
  ret.units = units;
  return ret;
}

smolforth_unit smolforth_unit_stack_pop(smolforth_unit_stack *stack) {
  return stack->units[stack->len-- - 1];
}

#define smolforth_unit_stack_require_throw(stack, n)                           \
  {                                                                            \
    if (stack->len < (n)) {                                                    \
      return SMOLFORTH_STATUS_STACK_UNDERFLOW;                                 \
    }                                                                          \
  }

void smolforth_unit_stack_push(smolforth_unit_stack *stack,
                               smolforth_unit unit) {
  stack->units[stack->len++] = unit;
}

typedef smolforth_status_ret (*smolforth_word_func_ptr)(smolforth_tok *, size_t,
                                                        smolforth_unit_stack *);

typedef struct smolforth_ctx;

#pragma region CORE FUNCTIONS
#ifdef SMOLFORTH_USE_EXT_DEFAULT_BUILTINS_LIST

smolforth_status_ret smolforth__word_dup(smolforth_tok *in, size_t in_len,
                                         smolforth_unit_stack *stack) {
  smolforth_unit_stack_require_throw(stack, 1);
  smolforth_unit u = smolforth_unit_stack_pop(stack);
  smolforth_unit_stack_push(stack, u);
  smolforth_unit_stack_push(stack, u);

  return SMOLFORTH_STATUS_OK;
}

smolforth_status_ret smolforth__word_swap(smolforth_tok *in, size_t in_len,
                                          smolforth_unit_stack *stack) {
  smolforth_unit_stack_require_throw(stack, 1);
  smolforth_unit a = smolforth_unit_stack_pop(stack);
  smolforth_unit b = smolforth_unit_stack_pop(stack);
  smolforth_unit_stack_push(stack, a);
  smolforth_unit_stack_push(stack, b);

  return SMOLFORTH_STATUS_OK;
}

smolforth_status_ret smolforth__word_noop(smolforth_tok *in, size_t in_len,
                                          smolforth_unit_stack *stack) {
  return SMOLFORTH_STATUS_OK;
}

smolforth_status_ret smolforth__word_drop(smolforth_tok *in, size_t in_len,
                                          smolforth_unit_stack *stack) {
  smolforth_unit_stack_require_throw(stack, 1);
  smolforth_unit_stack_pop(stack);
  return SMOLFORTH_STATUS_OK;
}

smolforth_status_ret smolforth__word_abs(smolforth_tok *in, size_t in_len,
                                         smolforth_unit_stack *stack) {
  smolforth_unit_stack_require_throw(stack, 1);
  smolforth_unit u = smolforth_unit_stack_pop(stack);

  switch (u.kind) {
  case SMOLFORTH_UNIT_DOUBLE:
    if (u.as_double < 0)
      u.as_double = -u.as_double;
    break;
  case SMOLFORTH_UNIT_INTEGER:
    if (u.as_integer < 0)
      u.as_integer = -u.as_integer;
    break;
  default:
    return SMOLFORTH_STATUS_TYPE_ERROR;
  }

  smolforth_unit_stack_push(stack, u);

  return SMOLFORTH_STATUS_OK;
}

smolforth_status_ret smolforth__word_mul(smolforth_tok *in, size_t in_len,
                                         smolforth_unit_stack *stack) {
  smolforth_unit_stack_require_throw(stack, 2);
  smolforth_unit a = smolforth_unit_stack_pop(stack);
  smolforth_unit b = smolforth_unit_stack_pop(stack);
  smolforth_unit c;

  if (a.kind == SMOLFORTH_UNIT_INTEGER && b.kind == SMOLFORTH_UNIT_DOUBLE) {
    c.kind = SMOLFORTH_UNIT_DOUBLE;
    c.as_double = (double)a.as_integer * b.as_double;
  } else if (a.kind == SMOLFORTH_UNIT_DOUBLE &&
             b.kind == SMOLFORTH_UNIT_DOUBLE) {
    c.kind = SMOLFORTH_UNIT_DOUBLE;
    c.as_double = a.as_double * b.as_double;
  } else if (a.kind == SMOLFORTH_UNIT_INTEGER &&
             b.kind == SMOLFORTH_UNIT_INTEGER) {
    c.kind = SMOLFORTH_UNIT_INTEGER;
    c.as_integer = a.as_integer * b.as_integer;
  } else if (a.kind == SMOLFORTH_UNIT_DOUBLE &&
             b.kind == SMOLFORTH_UNIT_INTEGER) {
    c.kind = SMOLFORTH_UNIT_DOUBLE;
    c.as_double = a.as_double * (double)b.as_integer;
  } else {
    abort();
  }

  smolforth_unit_stack_push(stack, c);

  return SMOLFORTH_STATUS_OK;
}

#endif
#pragma endregion

typedef struct _smolforth_kv_str_func_ptr_pair {
  char k[SMOLFORTH_MAX_WORD_LENGTH];
  smolforth_word_func_ptr v;
} _smolforth_kv_str_func_ptr_pair;

typedef struct smolforth_word_list {
  struct smolforth_word_list *parent;
  _smolforth_kv_str_func_ptr_pair pairs[SMOLFORTH_MAX_WORD_COUNT_IN_LIST];
  size_t len;
} smolforth_word_list;

void smolforth_word_list_append(smolforth_word_list *self, const char *name,
                                smolforth_word_func_ptr func) {
  memset(&self->pairs[self->len], 0, sizeof(char) * SMOLFORTH_MAX_WORD_LENGTH);
  size_t i = 0;
  for (i = 0; name[i]; i++)
    self->pairs[self->len].k[i] = name[i];

  self->pairs[self->len++].v = func;
}

smolforth_word_list smolforth_word_list_new(smolforth_word_list *parent) {
  smolforth_word_list ret;
  ret.len = 0;
  ret.parent = parent;
  return ret;
}

#ifdef SMOLFORTH_USE_EXT_DEFAULT_BUILTINS_LIST

typedef struct smolforth_ctx {
  smolforth_tok *tokens;
  size_t tokens_length;
  smolforth_unit_stack stack;
  smolforth_word_list wordbank;
} smolforth_ctx;

smolforth_ctx smolforth_ctx_new(smolforth_tok *toks, size_t toks_len,
                                smolforth_word_list *parent) {
  smolforth_ctx ctx;
  ctx.stack = smolforth_unit_stack_new(malloc(sizeof(smolforth_unit) * 16), 0);
  ctx.tokens = toks;
  ctx.tokens_length = toks_len;
  ctx.wordbank = smolforth_word_list_new(parent);
  return ctx;
}

smolforth_word_list smolforth_word_list_default() {
  smolforth_word_list ret = smolforth_word_list_new(NULL);
  smolforth_word_list_append(&ret, "dup", smolforth__word_dup);
  smolforth_word_list_append(&ret, "*", smolforth__word_mul);
  smolforth_word_list_append(&ret, "swap", smolforth__word_swap);
  smolforth_word_list_append(&ret, "noop", smolforth__word_noop);
  smolforth_word_list_append(&ret, "abs", smolforth__word_abs);
  smolforth_word_list_append(&ret, "drop", smolforth__word_drop);
  return ret;
}

#endif

smolforth_word_func_ptr smolforth_word_list_lookup(smolforth_word_list *self,
                                                   const char *name) {
  size_t i = 0;
  for (i = 0; i < self->len; i++)
    if (strcmp(name, self->pairs[i].k) == 0)
      return self->pairs[i].v;

  if (self->parent)
    return smolforth_word_list_lookup(self->parent, name);

  return NULL;
}

smolforth_status_ret smolforth_do_step(smolforth_ctx *ctx, size_t offset) {
  smolforth_tok_kind kind = ctx->tokens[offset].kind;
  switch (kind) {
  case SMOLFORTH_TOK_INTEGER:
    ctx->stack.units[ctx->stack.len].kind = SMOLFORTH_UNIT_INTEGER;
    ctx->stack.units[ctx->stack.len].as_integer =
        ctx->tokens[offset].as_integer;
    ctx->stack.len++;
    break;
  case SMOLFORTH_TOK_DOUBLE:
    ctx->stack.units[ctx->stack.len].kind = SMOLFORTH_UNIT_DOUBLE;
    ctx->stack.units[ctx->stack.len].as_double = ctx->tokens[offset].as_double;
    ctx->stack.len++;
    break;
  case SMOLFORTH_TOK_WORD: {
    smolforth_word_func_ptr func =
        smolforth_word_list_lookup(&ctx->wordbank, ctx->tokens[offset].as_word);

    if (func == NULL)
      return SMOLFORTH_STATUS_UNDEFINED_NAME;

    smolforth_status_ret ret =
        func(ctx->tokens + offset, ctx->tokens_length - offset, &ctx->stack);
    return ret;
  } break;
  }

  return SMOLFORTH_STATUS_OK;
}

smolforth_status_ret smolforth_do(smolforth_ctx *ctx) {
  size_t i = 0;
  for (i = 0; i < ctx->tokens_length; i++) {
    smolforth_status_ret ret = smolforth_do_step(ctx, i);
    if (ret != SMOLFORTH_STATUS_OK)
      return ret;
  }

  return SMOLFORTH_STATUS_OK;
}

#endif
