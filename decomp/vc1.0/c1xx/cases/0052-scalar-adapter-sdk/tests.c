/* The unary classifier is explicitly unit-qualified. The earlier frozen
   unary CLI draft reaches the original uninitialized folded-value defect. */
#include <recon-tool.h>
#include "operators.h"
#include <recon.h>
unsigned char __stdcall classify_unary_operator_token(unsigned char operation);
static int check_unary_operator_mapping(void)
{
    return classify_unary_operator_token(OP_ADD) == OP_UNARY_PLUS &&
        classify_unary_operator_token(OP_MULTIPLY) == OP_DEREFERENCE &&
        classify_unary_operator_token(OP_CALL) == 1;
}
const ReconTest scalar_adapter_tests[] = {
    {"unary token normalization (unit-only)", 0, check_unary_operator_mapping}
};
