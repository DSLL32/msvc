        case 325:
            parser_value.symbol = right[-1].symbol;
            ASSIGN_TYPE_SYMBOL(name_formal, 12);
            COPY_SYMBOL_STORAGE(right[-2]);
            normalize_symbol_type(right[-2].declaration.type, parser_value.symbol);
            --declarator_nesting;
            CLEAR_DECLARATION_FLAGS();
            POP_DECLARATION();
            break;
        case 326:
            parser_value.expression = right[-1].expression;
            POP_DECLARATION();
            break;
        case 327:
            parser_value.expression = 0;
            POP_DECLARATION();
            break;
        case 328:
            parser_value.expression = append_expression(0, right[0].expression, 0);
            break;
        case 329:
            parser_value.expression = append_expression(right[-2].expression, right[0].expression, 0);
            break;
        case 330:
            parser_value.context = lookup_segment_symbol(
                intern_identifier(right[-1].expression->value.text), name_unnamed, 8, 2);
            POP_DECLARATION();
            break;
        case 331:
            ALLOCATE_PARSER_NODE(parser_value.expression, sizeof(IrToken),
                (token_allocation_flags & 0x20000020UL) ? 0 : 2);
            parser_value.expression->flags = 5;
            parser_value.expression->kind = 3;
            parser_value.expression->value.pointer = right[0].context;
            parser_value.expression->tail.word = right[0].words.high;
            break;
        case 332:
            string_destination = (ParserValue *)&right[-1].expression->value;
            concatenate_strings(string_destination, right);
            break;
        case 333:
            new_array_bound = 0;
            cpp_parser_flags &= ~0x40;
            declaration_spec_flags[declaration_depth] |= 3;
            cpp_parser_flags &= ~1;
            break;
        case 334:
        case 335:
            parser_value.expression = new_expression(right[-3].bytes[0], right[-2].shape,
                0, right[-1].symbol, right[0].expression, new_array_bound);
            break;
        case 336:
        case 337:
            parser_value.expression = new_expression(right[-4].bytes[0], right[-3].shape,
                right[-2].expression, right[-1].symbol, right[0].expression, new_array_bound);
            break;
        case 338:
            cpp_parser_flags |= 0x41;
            CLEAR_DECLARATION_FLAGS();
            break;
        case 339:
            parser_value.symbol = right[0].symbol;
            detach_new_array_bound(parser_value.symbol);
            ASSIGN_TYPE_SYMBOL(name_new_target, 12);
            COPY_SYMBOL_STORAGE(right[-1]);
            normalize_symbol_type(right[-1].declaration.type, parser_value.symbol);
            if (has_cv_qualifiers(parser_value.symbol))
                report_type_error(468, parser_value.symbol->type);
            --declarator_nesting;
            declaration_spec_flags[declaration_depth] &= ~1;
            break;
        case 340:
            parser_value.symbol = new_abstract_declarator(0);
            ASSIGN_TYPE_SYMBOL(name_new_target, 12);
            normalize_symbol_type(right[0].symbol->type, parser_value.symbol);
            declaration_spec_flags[declaration_depth] &= ~1;
            break;
        case 341:
            parser_value.symbol = right[-1].symbol;
            detach_new_array_bound(parser_value.symbol);
            ASSIGN_TYPE_SYMBOL(name_cast, 12);
            COPY_SYMBOL_STORAGE(right[-2]);
            normalize_symbol_type(right[-2].declaration.type, parser_value.symbol);
            if (has_cv_qualifiers(parser_value.symbol))
                report_type_error(468, parser_value.symbol->type);
            --declarator_nesting;
            POP_DECLARATION();
            declaration_spec_flags[declaration_depth] &= ~1;
            break;
        case 342:
            parser_value.symbol = new_abstract_declarator(0);
            ++declarator_nesting;
            if (parser_lookahead == 0x80) declaration_spec_flags[declaration_depth] |= 3;
            break;
        case 343:
            parser_value.symbol = new_abstract_declarator(right[0].shape);
            ++declarator_nesting;
            break;
        case 344:
            parser_value.symbol = right[0].symbol;
            attach_type_shape(parser_value.symbol, new_type_shape(1, 0, 0));
            break;
        case 345:
            right[-2].shape->flags = 1;
            parser_value.symbol = right[0].symbol;
            attach_type_shape(parser_value.symbol, right[-2].shape);
            break;
        case 346:
            add_array_declarator(right[-3].symbol, right[-1].expression);
            break;
        case 347:
            parser_value.expression = 0;
            break;
        case 349:
            parser_value.expression = 0;
            POP_DECLARATION();
            break;
        case 350:
            parser_value.expression = delete_expression(right[-1].bytes[0], right[0].expression, 0, 0);
            break;
        case 351:
            parser_value.expression = delete_expression(right[-4].bytes[0], right[0].expression, right[-2].expression, 1);
            break;
        case 352:
        case 354:
            parser_value.bytes[0] = 1;
            break;
