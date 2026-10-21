#include "function-input.h"
#include "input.h"
extern FILE *aux_expression_file, *aux_symbol_file;
extern int floating_frontend;
unsigned pending_source_location = 1;
unsigned function_source_location, current_source_location;
int lexical_depth = -1;
int procedure_depth, end_function_records;

Function *__fastcall read_function(FunctionSymbol *symbol)
{
    return read_function_body(symbol);
}

/* Original [0004f140,0004f440). The two streams have independent offsets. */
Function *__fastcall read_function_body(FunctionSymbol *symbol)
{
    Function *function;
    Expression *head = 0, **tail = &head, *expression;
    Node *node, *child;
    FunctionSymbol *callee;
    TemporarySymbol *temporary;
    int auxiliary = symbol->flags52 & 1;
    function = arena_alloc(0, sizeof(Function));
    function->symbol = &symbol->symbol;
    function->scope = new_scope();
    input_file = auxiliary ? aux_symbol_file : symbol_file;
    fseek(input_file, symbol->field44, SEEK_SET);
    read_global_records(function);
    input_file = auxiliary ? aux_expression_file : expression_file;
    fseek(input_file, symbol->field40, SEEK_SET);
    pending_source_location = function_source_location = current_source_location = 0;
    while ((node = read_expression(function)) != 0) {
        switch (node->opcode) {
        case 0x2a: case 0x2b: case 0x7f:
            node->opcode = 1;
            break;
        case 0x41:
            ((FunctionSymbol *)function->symbol)->field24 |= 0x2000;
            break;
        case 0x46:
            function->root = node;
            child = node->left.node;
            if (child->opcode == 0x99) child->right = child->left.node->right;
            callee = child->right.function;
            function_source_location = current_source_location;
            ++procedure_depth;
            if (needs_return_temp(callee->symbol.field0e & 0x1c,
                                  (unsigned short)(callee->field24 & 0xf), callee->field28)) {
                temporary = create_temporary(function->scope, 0x400, 4, 3, 1, 4, 0);
                temporary->anonymous = 1;
                temporary->symbol.name = 0;
                function->return_temp = &temporary->symbol;
            }
            break;
        case 0x47: case 0x49:
            --procedure_depth;
            break;
        case 0x4b:
            child = node->left.node;
            release_node(node);
            node = child;
            break;
        case 0x4f:
            switch (node->detail.control.code) {
            case 0x16:
                function->field28 |= 1;
                decode_inline_assembly(node, function->scope);
                break;
            case 0x1f:
                function->options = node->left.value;
                node->opcode = 1;
                break;
            case 0x23:
                function->state = node->right.value;
                node->opcode = 1;
                break;
            }
            break;
        case 0x53:
            ++lexical_depth;
            if (lexical_depth > 1 && (!floating_frontend || procedure_depth)) {
                node->opcode = 0x4f;
                node->detail.control.code = 0xf;
                node->left.value = lexical_depth;
            } else node->opcode = 1;
            break;
        case 0x54:
            if (lexical_depth > 1 && (!floating_frontend || procedure_depth)) {
                node->opcode = 0x4f;
                node->detail.control.code = 0x10;
                node->left.value = lexical_depth;
            } else node->opcode = 1;
            if (!lexical_depth) end_function_records = 1;
            --lexical_depth;
            break;
        }
        if (node->opcode == 1) release_node(node);
        else {
            expression = new_expression();
            *tail = expression;
            expression->tree = node;
            current_source_location = pending_source_location;
            expression->source_location = pending_source_location;
            expression->next = 0;
            tail = &expression->next;
        }
        if (end_function_records) {
            end_function_records = 0;
            break;
        }
    }
    function->expressions = head;
    return head ? function : 0;
}

/* Original [0004f4b0,0004f570), including its calling-convention table. */
int __fastcall needs_return_temp(unsigned convention, unsigned short kind, unsigned bytes)
{
    switch (convention) {
    case 0: case 4: case 8: case 12: case 16: case 20:
        switch (kind) {
        case 5: return convention == 8;
        case 6: return floating_frontend || !is_register_return_size(bytes);
        case 8: return 1;
        default: return 0;
        }
    }
    /* As in the original, invalid convention encodings leave no result. */
}
