/* Shared edits of work-block and expression chains. */
#include "global-work.h"
#include "function-input.h"
#include <stdarg.h>

/* Original [00058db0,000590b0), including the 51-entry command table.
   Uppercase commands edit the doubly linked work chain; lowercase commands
   edit expressions. B and T reuse the current cursor instead of consuming
   the next argument. R/r select which cursor is returned. */
void *__cdecl build_work_lists(const char *format, ...)
{
    va_list arguments;
    WorkItem *work = 0, *following_work;
    Expression *expression = 0, *following_expression, *inserted, **link;
    void *result = 0;
    int use_work = 0, use_expression = 0;
    va_start(arguments, format);
    for (; *format; ++format) {
        switch (*format) {
        case 'A':
            work = work->next;
            break;
        case 'B':
            use_work = 1;
            break;
        case 'H':
            if (use_work) {
                use_work = 0;
                work->next = work;
            } else work->next = va_arg(arguments, WorkItem *);
            break;
        case 'I':
            following_work = work->next;
            if (use_work) {
                use_work = 0;
                work->next = work;
            } else work->next = va_arg(arguments, WorkItem *);
            work->next->previous = work;
            work = work->next;
            work->next = following_work;
            following_work->previous = work;
            break;
        case 'L':
            if (use_expression) {
                use_expression = 0;
                work->body.expressions = expression;
            } else work->body.expressions = va_arg(arguments, Expression *);
            expression = work->body.expressions;
            break;
        case 'N':
            following_work = work->next;
            work->next = new_work_item();
            work->next->previous = work;
            work->next->region = work->region;
            work = work->next;
            work->next = following_work;
            if (following_work) following_work->previous = work;
            break;
        case 'P':
            link = va_arg(arguments, Expression **);
            following_expression = *link;
            inserted = new_expression();
            *link = inserted;
            inserted->source_location = following_expression->source_location;
            inserted->tree = va_arg(arguments, Node *);
            inserted->next = following_expression;
            break;
        case 'R':
            result = work;
            break;
        case 'S':
            if (use_work) use_work = 0;
            else work = va_arg(arguments, WorkItem *);
            break;
        case 'T':
            use_expression = 1;
            break;
        case 'a':
            expression = expression->next;
            break;
        case 'h':
            if (use_expression) {
                use_expression = 0;
                expression->next = expression;
            } else expression->next = va_arg(arguments, Expression *);
            break;
        case 'i':
            following_expression = expression->next;
            if (use_expression) {
                expression->next = expression;
                expression = expression->next;
                use_expression = 0;
            } else {
                expression->next = va_arg(arguments, Expression *);
                expression = expression->next;
            }
            expression->next = following_expression;
            break;
        case 'l':
            expression->tree = va_arg(arguments, Node *);
            break;
        case 'n':
            if (expression) {
                following_expression = expression->next;
                expression->next = new_expression();
                expression->next->source_location = expression->source_location;
                expression = expression->next;
                expression->next = following_expression;
            } else {
                expression = new_expression();
                expression->source_location = current_source_location;
            }
            break;
        case 'r':
            result = expression;
            break;
        case 's':
            if (use_expression) use_expression = 0;
            else expression = va_arg(arguments, Expression *);
            break;
        }
    }
    va_end(arguments);
    return result;
}

/* Original [0004fd10,0004fd90). Insert before the indicated tree, appending
   when it is absent. Preserve the head record's address when inserting first:
   move its old contents to the new record and clear the new head's metadata. */
Expression *__fastcall insert_tree_expression(Expression *expressions, Node *before, Node *tree)
{
    Expression *inserted = new_expression(), *previous;
    inserted->tree = tree;
    if (!expressions) return inserted;
    if (expressions->tree == before) {
        *inserted = *expressions;
        expressions->next = inserted;
        expressions->tree = tree;
        expressions->field08 = 0;
        expressions->source_location = 0;
    } else {
        previous = expressions;
        while (previous->next && previous->next->tree != before)
            previous = previous->next;
        inserted->next = previous->next;
        previous->next = inserted;
    }
    return expressions;
}
