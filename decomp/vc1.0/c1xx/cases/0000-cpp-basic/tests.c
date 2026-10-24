/* Narrow native policy checks. Synthetic unsupported inputs establish rejection,
 * not equivalence or public reachability. Historical proof: tagged 17c78f8c6. */
#include <sys/wait.h>
#include <unistd.h>
#include <recon-tool.h>
#include "debug-types.h"
#include "debug-info.h"
#include "unary-expressions.h"
#include "class-members.h"
#include "binary-expressions.h"
#include "parser.h"
#include "lexer.h"
#include "browser.h"
#include <recon.h>

static int traps(void (*invoke)(void))
{
    int child = fork(), status;
    if (child < 0) return 0;
    if (!child) { invoke(); _exit(99); }
    if (waitpid(child, &status, 0) != child) return 0;
    return WIFSIGNALED(status) && WTERMSIG(status) == SIGTRAP;
}
static void single_null(void) { has_single_function(0); }
static void overload_null(void) { count_emitted_overloads(0); }
static void methods_null(void) { unsigned short flags = 0; count_debug_methods(0, &flags); }
static void method_list_null(void)
{
    static unsigned char bytes[4096];
    debug_record_buffer = bytes;
    emit_debug_method_list(0);
}
static void real_width(void) { primitive_debug_type(0x10b); }
static void integer_width(void) { primitive_debug_type(0x49); }
static void cached_lookahead(void) { parser_lookahead = 2; begin_implicit_statement_scope(0); }
static void multiply_width(void) { multiplication_overflows(1, ~0U, 0, 3); }
static void empty_browser_name(void)
{
    char result[16]; options.option_Fr = ""; format_browser_temporary_name(result);
}
static void unknown_browser_category(void)
{
    BrowserRecord record = {0}; CppSymbol symbol = {0};
    symbol.name = "plain"; symbol.kind = 12; symbol.declaration_flags = 0x200;
    fill_browser_declaration(&record, &symbol);
}
#define TRAP_TEST(name, body) static int name(void) { return traps(body); }
TRAP_TEST(check_single_null, single_null)
TRAP_TEST(check_overload_null, overload_null)
TRAP_TEST(check_methods_null, methods_null)
TRAP_TEST(check_method_list_null, method_list_null)
TRAP_TEST(check_real_width, real_width)
TRAP_TEST(check_integer_width, integer_width)
TRAP_TEST(check_cached_lookahead, cached_lookahead)
TRAP_TEST(check_multiply_width, multiply_width)
TRAP_TEST(check_empty_browser_name, empty_browser_name)
TRAP_TEST(check_unknown_browser_category, unknown_browser_category)

/* These ten returns were observed directly in original and native at the
 * existing multiply boundary. Keep concrete independent expectations. */
static int check_multiply_returns(void)
{
    static const unsigned values[][5] = {
        {0U,4U,0U,4U,0}, {255U,2U,1U,1U,1},
        {4294967168U,1U,0U,1U,0}, {4294967168U,4294967295U,0U,1U,1},
        {4294934528U,1U,0U,2U,0}, {4294934528U,4294967295U,0U,2U,1},
        {2147483648U,1U,0U,4U,0}, {2147483648U,4294967295U,0U,4U,1},
        {2147483647U,2U,0U,4U,1}, {4294967295U,2U,1U,4U,1}
    };
    unsigned i;
    for (i=0; i<RECON_COUNT(values); ++i)
        if (multiplication_overflows(values[i][0],values[i][1],(short)values[i][2],
                (unsigned short)values[i][3]) != (int)values[i][4]) return 0;
    return 1;
}
static int check_primitive_types(void)
{
    return primitive_debug_type(0x44)==0x12 && primitive_debug_type(0x64)==0x74 &&
        primitive_debug_type(0x80)==3 && primitive_debug_type(0)==0;
}
static int check_browser_filename(void)
{
    char result[16];
    options.option_Fr="a.sbr"; format_browser_temporary_name(result);
    if (strcmp(result,"a.sbt")) return 0;
    options.option_Fr="a.sbt"; format_browser_temporary_name(result);
    return strcmp(result,"a.sbu")==0;
}
static int check_single_function(void)
{
    CppSymbol first={0}, second={0};
    first.kind=6;
    if (has_single_function(&first)!=1) return 0;
    first.next=&second; second.kind=7;
    if (has_single_function(&first)!=0) return 0;
    second.kind=3;
    return has_single_function(&first)==1;
}
const ReconTest c1xx_tests[] = {
    {"NULL single-function guard",1,check_single_null},
    {"NULL overload-count guard",1,check_overload_null},
    {"NULL debug-method-count guard",1,check_methods_null},
    {"NULL debug-method-list guard",1,check_method_list_null},
    {"unknown real width guard",1,check_real_width},
    {"unknown integer width guard",1,check_integer_width},
    {"cached parser lookahead guard",1,check_cached_lookahead},
    {"unsupported signed multiply width guard",1,check_multiply_width},
    {"empty browser filename guard",1,check_empty_browser_name},
    {"unmodeled browser category guard",1,check_unknown_browser_category},
    {"ten original-backed multiply returns",1,check_multiply_returns},
    {"primitive CodeView types",1,check_primitive_types},
    {"browser filename suffixes",1,check_browser_filename},
    {"single-function list controls",1,check_single_function}
};
