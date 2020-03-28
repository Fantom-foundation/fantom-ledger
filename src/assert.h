#ifndef FANTOM_LEDGER_ASSERT_H
#define FANTOM_LEDGER_ASSERT_H

// convenience redefinition of c keyword
#define STATIC_ASSERT _Static_assert

// declare the actual function we use for assertion processing
extern void assert(int cond, const char *msgStr);

// substitutes function above, but it's more eye catching
#define ASSERT_WITH_MSG(cond, msg) assert(cond, msg)

// max length of an assert we allow
#define _MAX_ASSERT_LENGTH_ 25

// make sure we don't show more than _MAX_ASSERT_LENGTH_ chars; if so cat the beginning and show the tail
#define _ASSERT_MSG_LEN_(strLiteral, size) (sizeof(strLiteral) > size ? (strLiteral) + sizeof(strLiteral) - size : strLiteral)

// from https://stackoverflow.com/questions/19343205/c-concatenating-file-and-line-macros
#define _TO_STR1_(x) #x
#define _TO_STR2_(x) _TO_STR1_(x)
#define _FILE_LINE_ __FILE__ ":" _TO_STR2_(__LINE__)

// the basic ASSERT will work with file line
#define ASSERT(cond) assert((cond), _ASSERT_MSG_LEN_(_FILE_LINE_, _MAX_ASSERT_LENGTH_))
#endif //FANTOM_LEDGER_ASSERT_H
