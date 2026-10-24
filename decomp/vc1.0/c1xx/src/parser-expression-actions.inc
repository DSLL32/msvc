        case 248:
            deferred_declarations[declaration_depth] = 0;
            break;
        case 250:
            default_label = 0;
            break;
        case 251:
            token_allocation_flags &= ~1;
            CLEAR_DECLARATION_FLAGS();
            break;
        case 252:
            if (normalized_Ol && options.configuration.option_qc) record_qc_jump(0);
            label_name = right[-1].name;
            define_named_label(label_name);
            break;
        case 253:
            if (!right[-1].expression || !default_labels) report_syntax_error(46);
            else {
                if (options.configuration.option_qc) current_qc_context = 0;
                case_expression = validate_value_expression(right[-1].expression);
                add_case_label(case_expression);
            }
            break;
        case 254:
            if (options.configuration.option_qc) current_qc_context = 0;
            if (!switch_labels) report_syntax_error(47);
            else if (switch_labels->label) report_syntax_error(48);
            else {
                if (!default_label) {
                    switch_labels->label = new_branch_label(0x14);
                    default_label = switch_labels->label;
                    emit_label(switch_labels->label);
                } else switch_labels->label = default_label;
                record_default_label();
            }
            break;
        case 255:
            begin_if_condition();
            declaration_spec_flags[declaration_depth] |= 3;
            break;
        case 256:
            parser_value.expression = new_branch_label(0x1e);
            if (right[-1].expression) {
                right[-1].expression = validate_value_expression(right[-1].expression);
                right[-1].expression = boolean_expression(right[-1].expression, 0);
                emit_condition_to_label(right[-1].expression, parser_value.expression, 0);
            }
            begin_if_body();
            POP_DECLARATION();
            declaration_spec_flags[declaration_depth] &= ~1;
            begin_implicit_statement_scope(parser_value.expression);
            break;
        case 257:
            finish_implicit_statement_scope(right[-2].expression, 0);
            begin_else_body();
            parser_value.expression = new_branch_label(0x1e);
            write_ir_record(0x3a, parser_value.expression);
            emit_label(right[-2].expression);
            record_line_information(1);
            declaration_spec_flags[declaration_depth] &= ~1;
            begin_implicit_statement_scope(parser_value.expression);
            break;
        case 258:
        case 261:
            parser_value.expression = 0;
            break;
        case 259:
            if ((current_class_context < 0 || !options.configuration.extensions_enabled) && !right[0].words.low)
                report_syntax_error(466);
            break;
        case 260:
            parser_value.words.low = evaluate_integer_constant(validate_value_expression(right[0].expression));
            break;
        case 262:
            parser_value.expression = validate_value_expression(right[0].expression);
            break;
        case 263:
        case 266:
        case 274:
        case 275:
        case 276:
        case 277:
        case 278:
        case 279:
        case 280:
        case 281:
        case 282:
        case 283:
        case 285:
            parser_value.expression = binary_expression(right[-1].bytes[0], right[-2].expression, right[0].expression);
            break;
        case 264:
            report_syntax_error(290, token_spellings[189]);
            parser_value.expression = right[0].expression;
            break;
        case 265:
            if (!right[-2].expression || !right[0].expression) parser_value.expression = 0;
            else {
                expression = resolve_assignment_operand(right[-2].expression->type, right[0].expression);
                if (!expression) report_expression_error(0x3d, right[0].expression);
                parser_value.expression = binary_expression(right[-1].bytes[0], right[-2].expression, expression);
            }
            break;
        case 267:
        case 268:
        case 270:
        case 272:
            if (right[-1].expression) mark_evaluation_boundary();
            break;
        case 269:
            parser_value.expression = conditional_expression(right[-6].expression, right[-3].expression, right[0].expression);
            break;
        case 271:
        case 273:
            parser_value.expression = binary_expression(right[-2].bytes[0], right[-3].expression, right[0].expression);
            break;
        case 284:
            parser_value.expression = segment_offset_expression(right[-2].expression, right[0].expression);
            break;
        case 288:
            cpp_parser_flags |= 2;
            parser_value.expression = cast_expression(right[-1].symbol, right[0].expression);
            cpp_parser_flags &= ~2;
            clear_reference_cache(parser_value.expression);
            break;
        case 290:
            right[-1].bytes[0] = right[-1].bytes[0] == OP_POST_INCREMENT ? OP_PRE_INCREMENT : OP_PRE_DECREMENT;
            parser_value.expression = binary_expression(right[-1].bytes[0], right[0].expression, make_integer_constant(1));
            break;
        case 291:
            parser_value.expression = unary_expression(OP_DEREFERENCE, right[0].expression);
            break;
        case 292:
            expression = right[0].expression;
            if (!expression) parser_value.expression = 0;
            else if (expression->kind == OP_DEREFERENCE && EXPR(expression)->right)
                parser_value.expression = member_pointer_expression((QualifiedDeclarator *)EXPR(expression)->right);
            else if (expression->kind == OP_ARROW && (EXPR(expression)->right->flags & 7) == 4 &&
                     EXPR(EXPR(expression)->right)->right)
                parser_value.expression = unary_expression(OP_BIT_AND, EXPR(expression)->right);
            else {
                cpp_parser_flags |= 0x10;
                parser_value.expression = unary_expression(OP_BIT_AND, right[0].expression);
                cpp_parser_flags &= ~0x10;
            }
            break;
        case 293:
            parser_value.expression = unary_expression(OP_UNARY_PLUS, right[0].expression);
            clear_reference_cache(parser_value.expression);
            break;
        case 294:
            parser_value.expression = unary_expression(OP_NEGATE, right[0].expression);
            break;
        case 295:
            parser_value.expression = unary_expression(OP_NOT, right[0].expression);
            break;
        case 296:
            token_allocation_flags &= ~1;
            break;
        case 297:
            parser_value.expression = unary_expression(OP_COMPLEMENT, right[0].expression);
            break;
        case 298:
            parser_value.expression = right[0].expression ? sizeof_expression(right[0].expression) : 0;
            --allow_incomplete_class;
            break;
        case 299:
            parser_value.expression = sizeof_type(right[0].symbol->type);
            --allow_incomplete_class;
            break;
        case 304:
            parser_value.expression = binary_expression(right[-2].bytes[0], right[-3].expression, right[-1].expression);
            break;
        case 305:
            declaration_spec_flags[declaration_depth] |= 3;
            break;
        case 306:
            parser_value.expression = finish_call_expression(
                build_call_expression(right[-2].expression, right[0].expression), 0);
            break;
        case 307:
            member_lookup_context = begin_member_lookup(right[0].bytes[0], right[-1].expression, 0);
            member_lookup_operator = right[0].bytes[0];
            break;
        case 308:
            parser_value.expression = right[0].expression;
            break;
        case 309:
            parser_value.expression = binary_expression(right[0].bytes[0], right[-1].expression, make_integer_constant(1));
            break;
        case 310:
            parser_value.expression = operator_operand_expression(right[-1].bytes[0], right[0].expression);
            break;
        case 312:
            record_string_literal(right[0].expression);
            break;
        case 313:
            parser_value.expression = validate_based_self();
            break;
        case 314:
            parser_value.expression = builtin_unary_expression(OP_CALL, right[-1].expression);
            clear_reference_cache(parser_value.expression);
            POP_DECLARATION();
            break;
        case 315:
            parser_value.expression = new_unary_ir_expression(0x1b, right[0].symbol);
            break;
        case 317:
            parser_value.expression = member_expression(member_lookup_operator, member_lookup_context, right[0].expression);
            break;
        case 318:
            parser_value.expression = member_destructor_expression(member_lookup_operator, member_lookup_context, right[0].name);
            break;
        case 319:
            parser_value.expression = identifier_expression(right[0].name);
            break;
        case 320:
            parser_value.expression = identifier_expression((char *)right[0].words.high);
            break;
        case 321:
            parser_value.expression = identifier_expression(encode_type_name(right[0].declaration.type));
            token_allocation_flags &= ~4;
            break;
        case 322:
            unwind_qualified_scopes(1);
            parser_value.expression = symbol_expression(right[0].context);
            break;
        case 323:
            unwind_qualified_scopes(1);
            qualified = right[-1].context;
            if (qualified) {
                parser_name = encode_type_name(right[0].declaration.type);
                qualified->resolution = lookup_qualified_member(qualified->resolution,
                    parser_name, right[0].declaration.type);
                parser_value.expression = symbol_expression((CppSymbol *)qualified);
            }
            token_allocation_flags &= ~4;
            break;
