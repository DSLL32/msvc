        case 3: /* external_definition */
            finish_external_definition(parser_recovery_count);
            break;
        case 4: /* unexpected top-level brace */
            if (!(cpp_parser_flags & 0x100)) report_syntax_error(447);
            else cpp_parser_flags &= ~0x100;
            skip_balanced_brace_body();
            if (source_text_cursor) ++source_text_cursor;
            parser_recovery_count = 0;
            parser_lookahead = -1;
            break;
        case 8:
            declare_nctag(right[-1].name);
            break;
        case 9:
            parser_recovery_count = 0;
            break;
        case 10: /* untyped_function_begin */
            token_allocation_flags |= 0x02000000;
            cpp_parser_flags &= ~4;
            begin_function_definition(current_declaration_type(0), right[0].symbol);
            --declarator_nesting;
            CLEAR_DECLARATION_FLAGS();
            break;
        case 11:
            apply_constructor_initializers(right[0].expression);
            cpp_parser_flags &= ~0x100;
            break;
        case 12:
            end_function_definition();
            break;
        case 13: /* typed_function_declarator */
            if (!declaration_mode) {
                token_allocation_flags &= ~0x02000000;
                cpp_parser_flags &= ~4;
                begin_function_definition(&right[-1].declaration, right[0].symbol);
            } else {
                report_syntax_error(599, right[0].symbol->name);
                if (parser_lookahead != 0x7f) {
                    parser_lookahead = synchronize_tokens((unsigned char)parser_lookahead, 0x7f, 0x8a, 1);
                    if (parser_lookahead == 0x8a) cpp_parser_flags |= 0x2000;
                }
                parser_value.symbol = 0;
            }
            --declarator_nesting;
            CLEAR_DECLARATION_FLAGS();
            break;
        case 14:
            parser_value.expression = 0;
            break;
        case 15: /* constructor_initializer_begin */
            token_allocation_flags |= 1;
            cpp_parser_flags |= 0x1000;
            break;
        case 16:
            if (right[0].bytes[0] != 1)
                report_syntax_error(612, token_spellings[right[0].bytes[0]]);
            token_allocation_flags &= ~1;
            cpp_parser_flags &= ~0x1000;
            parser_value.expression = right[-1].expression;
            break;
        case 17:
            parser_value.expression = append_constructor_initializer(0, right[0].expression);
            break;
        case 18:
            parser_value.expression = append_constructor_initializer(right[-2].expression, right[0].expression);
            break;
        case 19:
            report_syntax_error(630, token_spellings[0x8a]);
            parser_value.expression = append_constructor_initializer(right[-2].expression, right[0].expression);
            break;
        case 20:
            declaration_spec_flags[declaration_depth] |= 3;
            break;
        case 21:
            parser_value.expression = new_ir_expression(OP_CALL,
                symbol_reference(new_declarator(right[-4].name)), right[-1].expression, 0);
            token_allocation_flags |= 1;
            POP_DECLARATION();
            break;
        case 22:
            parser_value.expression = 0;
            report_syntax_error(351);
            token_allocation_flags |= 1;
            break;
        case 23:
            parser_value.expression = 0;
            report_syntax_error(351);
            token_allocation_flags |= 1;
            POP_DECLARATION();
            break;
        case 24:
            parser_value.expression = 0;
            parser_recovery_count = 0;
            synchronize_tokens((unsigned char)parser_lookahead, 0x7f, 0x8a, 1);
            break;
        case 25:
            parser_value.expression = right[-1].expression;
            POP_DECLARATION();
            break;
        case 26:
            parser_value.expression = 0;
            break;
        case 28:
            parser_value.expression = append_expression(0, right[0].expression, 2);
            break;
        case 29:
            parser_value.expression = append_expression(right[-2].expression, right[0].expression, 2);
            break;
        case 31:
            pop_linkage();
            break;
        case 33:
            if (!right[-1].symbol) token_allocation_flags |= 0x01000000;
            else apply_constructor_initializers(right[0].expression);
            cpp_parser_flags &= ~0x100;
            break;
        case 34:
            if (!right[-3].symbol) token_allocation_flags &= ~0x01000000;
            else end_function_definition();
            break;
        case 35:
            parser_recovery_count = 0;
            cpp_parser_flags &= ~0x100;
            token_allocation_flags &= ~0x01000000;
            break;
        case 36:
            check_class_specifier(&right[-1].declaration);
            break;
        case 37:
            check_class_specifier(&right[0].declaration);
            check_empty_declaration(&right[0].declaration);
            break;
        case 39:
            cpp_parser_flags &= ~0x80;
            break;
        case 40:
            CLEAR_DECLARATION_FLAGS();
            break;
        case 41:
            push_linkage(parse_linkage(&right[-2].declaration, right[0].expression));
            RESET_EXPRESSION_ARENA();
            CLEAR_DECLARATION_FLAGS();
            if (parser_lookahead != 0x7f) cpp_parser_flags |= 0x80;
            break;
        case 44:
            report_syntax_error(290, token_spellings[0xba]);
            break;
        case 45:
            if (parser_lookahead == 0x80) {
                declaration_spec_stack[declaration_depth-1] = right[0].declaration;
                declaration_spec_flags[declaration_depth-1] |= 1;
            } else {
                declaration_spec_stack[declaration_depth] = right[0].declaration;
                declaration_spec_flags[declaration_depth] |= 1;
            }
            break;
        case 47:
            cpp_parser_flags |= 1;
            current_token_value = parser_value;
            read_type_specifier(0x26);
            cpp_parser_flags &= ~1;
            parser_value = current_token_value;
            break;
        case 48:
            parser_value.declaration.type = end_class_specifier();
            flush_scanned_declaration(&parser_value.declaration);
            break;
        case 49:
            parser_value.declaration.type = end_enum_specifier();
            break;
        case 50:
            begin_class_body();
            break;
        case 52:
            token_allocation_flags |= 3;
            break;
        case 53:
            if (right[0].bytes[0] != 1)
                report_syntax_error(613, token_spellings[right[0].bytes[0]]);
            token_allocation_flags &= ~3;
            begin_class_body();
            break;
        case 55:
        case 56:
            token_allocation_flags &= ~1;
            break;
        case 59:
            add_named_base((unsigned char)(right[-1].bytes[0] & 3),
                           -(int)(right[-1].words.high & 1), right[0].name);
            token_allocation_flags |= 1;
            break;
        case 60:
            unwind_qualified_scopes(1);
            resolve_deferred_declaration(right[0].symbol);
            add_qualified_base((unsigned char)(right[-1].bytes[0] & 3),
                               -(int)(right[-1].words.high & 1), right[0].context);
            token_allocation_flags |= 1;
            break;
        case 61:
            parser_recovery_count = 0;
            unget_token(0x7f);
            break;
        case 62:
            parser_value.words.low &= ~3;
            parser_value.words.high &= ~7;
            break;
        case 63:
            if (!(parser_value.bytes[0] & 3)) {
                READ_DECLARATION_CODE(attributes, right[0].bytes[0]);
                parser_value.bytes[0] |= attributes & 3;
            }
            else if (!(parser_value.words.high & 2)) {
                report_syntax_error(502, CURRENT_CLASS_SYMBOL->name);
                parser_value.words.high |= 2;
            }
            break;
        case 64:
            if (!(parser_value.words.high & 1)) parser_value.words.high |= 1;
            else if (!(parser_value.words.high & 4)) {
                report_syntax_error(507, CURRENT_CLASS_SYMBOL->name);
                parser_value.words.high |= 4;
            }
            break;
        case 67:
            enumerator_value = next_enumerator_value(enumerator_value, right[0].name);
            declare_enumerator(right[0].name, enumerator_context, enumerator_value);
            token_allocation_flags |= 1;
            break;
        case 68:
            declare_enumerator(right[-2].name, enumerator_context, right[0].words.low);
            if (!suppress_implicit_member && !type_description_depth &&
                !(token_allocation_flags & 0x10000000)) release_arena(2);
            token_allocation_flags |= 1;
            break;
        case 69:
        case 70:
            if (!suppress_implicit_member && !type_description_depth &&
                !(token_allocation_flags & 0x10000000)) release_arena(2);
            token_allocation_flags &= ~1;
            text_work_cursor = expression_storage;
            CLEAR_DECLARATION_FLAGS();
            break;
        case 72:
            declare_class_member(&right[-1].declaration, 0);
            break;
        case 74:
            check_class_specifier(&right[-2].declaration);
            break;
        case 77:
            READ_DECLARATION_CODE(attributes, right[-1].bytes[0]);
            set_member_access((unsigned char)attributes);
            break;
        case 78:
            report_syntax_error(238, token_spellings[0x3f]);
            parser_recovery_count = 0;
            break;
        case 79:
            report_syntax_error(238, token_spellings[0x8a]);
            parser_recovery_count = 0;
            break;
        case 82:
        case 83:
            parser_lookahead = synchronize_tokens((unsigned char)parser_lookahead, 0x8a, 0x85, 1);
            if (parser_lookahead != 0x8a) {
                unget_token((unsigned char)parser_lookahead);
                parser_lookahead = 0x8a;
            }
            parser_recovery_count = 0;
            break;
        case 84:
            parser_value.symbol->kind = 3;
            --declarator_nesting;
            declare_class_member(current_declaration_type(0), parser_value.symbol);
            break;
        case 85:
            validate_pure_specifier(validate_value_expression(right[0].expression));
            parser_value.symbol->kind = 0x13;
            --declarator_nesting;
            declare_class_member(current_declaration_type(0), parser_value.symbol);
            break;
        case 86:
            parser_value.symbol->kind = 2;
            parser_value.symbol->auxiliary.value =
                (parser_value.symbol->auxiliary.value & ~0xff00) | ((unsigned)right[0].bytes[0] << 8);
            --declarator_nesting;
            declare_class_member(current_declaration_type(0), parser_value.symbol);
            break;
        case 87:
            parser_value.symbol = new_abstract_declarator(0);
            parser_value.symbol->kind = 2;
            parser_value.symbol->name = name_alignment;
            parser_value.symbol->flags |= 0x80;
            parser_value.symbol->auxiliary.value =
                (parser_value.symbol->auxiliary.value & ~0xff00) | ((unsigned)right[0].bytes[0] << 8);
            declare_class_member(current_declaration_type(0), parser_value.symbol);
            break;
        case 88:
            declare_constructor(current_declaration_type(right[-1].bytes),
                cpp_context_stack->items[((ClassReference *)right[-1].type_name.type)->context_index],
                right[0].symbol);
            break;
        case 91:
            --declarator_nesting;
            if (!(cpp_parser_flags & 0x100))
                right[0].symbol = complete_declaration(current_declaration_type(0), right[0].symbol);
            unwind_qualified_scopes(1);
            break;
        case 92:
            if (!options.prototypes_only && !reject_function_initializer(right[-1].symbol)) {
                --declarator_nesting;
                right[-1].symbol->attributes |= 4;
                right[-1].symbol = apply_declaration_type(current_declaration_type(0), right[-1].symbol);
                right[-1].symbol = begin_object_initializer(right[-1].symbol);
                if (right[-1].symbol) {
                    mark_initialized_symbol(right[-1].symbol);
                    initialize_object(right[-1].symbol, right[0].expression);
                    unwind_qualified_scopes(1);
                    finish_object_initializer(right[-1].symbol);
                }
            }
            text_work_cursor = expression_storage;
            break;
        case 93:
            if (!options.prototypes_only && right[-1].symbol) {
                mark_initialized_symbol(right[-1].symbol);
                initialize_object(right[-1].symbol, validate_value_expression(right[0].expression));
                unwind_qualified_scopes(1);
                finish_object_initializer(right[-1].symbol);
            }
            text_work_cursor = expression_storage;
            break;
        case 94:
            if (!options.prototypes_only) {
                begin_braced_initializer(right[0].symbol);
                initializer_list_started = 1;
            }
            break;
        case 95:
            end_braced_initializer(right[-5].symbol);
            unwind_qualified_scopes(1);
            if (!options.prototypes_only) finish_object_initializer(right[-5].symbol);
            break;
        case 96:
            --declarator_nesting;
            if (!options.prototypes_only) {
                right[-1].symbol->attributes |= 4;
                parser_value.symbol = apply_declaration_type(current_declaration_type(0), right[-1].symbol);
                parser_value.symbol = begin_object_initializer(parser_value.symbol);
                declaration_spec_flags[declaration_depth] |= 2;
            }
            break;
        case 97:
            report_missing_declarator(0x201, current_declaration_type(0));
            parser_value.symbol = 0;
            break;
        case 100:
            if (!options.prototypes_only)
                initialize_element(validate_value_expression(right[0].expression), 0);
            text_work_cursor = expression_storage;
            break;
