        case 101:
            if (!options.prototypes_only) begin_nested_initializer();
            break;
        case 102:
            if (!options.prototypes_only) end_nested_initializer();
            break;
        case 103:
            ++declarator_nesting;
            break;
        case 104:
            parser_value.symbol = attach_function_parameters(right[-1].symbol, right[0].symbol);
            break;
        case 105:
            parser_value.symbol = right[0].symbol;
            right[-2].shape->flags = 1;
            attach_type_shape(parser_value.symbol, right[-2].shape);
            break;
        case 106:
            parser_value.symbol = right[0].symbol;
            right[-2].shape->flags = 0x40;
            attach_type_shape(parser_value.symbol, right[-2].shape);
            break;
        case 107:
            parser_value.symbol = right[0].symbol;
            resolve_member_pointer_class(&member_class, right[-1].context);
            if (!member_class) right[-2].shape->flags = 1;
            else {
                right[-2].shape->flags = 0x10;
                right[-2].shape->data.tag_symbol = member_class;
            }
            attach_type_shape(parser_value.symbol, right[-2].shape);
            break;
        case 108:
            array_bound = right[-1].words.low;
            attach_type_shape(parser_value.symbol, new_type_shape(2, 0, &array_bound));
            break;
        case 109:
            parser_value.symbol = right[-1].symbol;
            POP_DECLARATION();
            break;
        case 110:
            parser_value.symbol = apply_declarator_modifier(right[0].symbol, right[-1].shape);
            break;
        case 112:
            parser_value.symbol = new_declarator(right[0].name);
            if (is_current_class(parser_value.symbol)) parser_value.symbol->declaration_flags |= 0x1000;
            parser_value.symbol->cpp.parts.token = 1;
            break;
        case 114:
            parser_value.symbol = new_declarator(right[0].name);
            if (is_current_class(parser_value.symbol)) parser_value.symbol->declaration_flags |= 0x2000;
            else if (current_class_context < 0)
                report_syntax_error(588, parser_value.symbol->name);
            else {
                report_syntax_error(523, current_class_name(), right[0].name);
                parser_value.symbol->name = current_class_name();
                parser_value.symbol->declaration_flags |= 0x2000;
            }
            parser_value.symbol->cpp.parts.token = 1;
            break;
        case 115:
            parser_value.symbol = new_declarator((char *)right[0].words.high);
            parser_value.symbol->cpp.parts.token = right[0].bytes[0];
            break;
        case 116:
            conversion_declaration = right[0].declaration;
            parser_value.symbol = new_declarator(encode_type_name(conversion_declaration.type));
            parser_value.symbol->cpp.parts.token = 0x9a;
            parser_value.symbol->declaration_flags |= 0x4000;
            token_allocation_flags &= ~4;
            break;
        case 117:
            qualified = right[0].context;
            if (qualified) {
                symbol = (CppSymbol *)qualified->resolution->binding->target;
                parser_value.symbol = new_declarator(qualified->name);
                parser_value.symbol->next = (CppSymbol *)qualified;
                if (qualified->declaration_flags & 0x4000) {
                    /* Conversion names may designate an overload-set wrapper.
                       Its common +0xc slot is a function list, not a CType. */
                    symbol = ((OverloadSet *)symbol)->functions;
                    type = symbol->type;
                    description = (TypeDescription *)drop_first_type_shape(type);
                    if (!(type->related->flags & 0x77))
                        description->related = description->related->related;
                    conversion_declaration.type = intern_type(description);
                    conversion_declaration.storage[0] = symbol->storage[0];
                    conversion_declaration.storage[1] = symbol->storage[1];
                    conversion_declaration.storage[2] = symbol->storage[2];
                    parser_value.symbol->declaration_flags |= 0x4000;
                } else if (qualified->declaration_flags & 0x1000)
                    parser_value.symbol->declaration_flags |= 0x1000;
                else if (qualified->declaration_flags & 0x2000)
                    parser_value.symbol->declaration_flags |= 0x2000;
                if ((symbol->kind & 31) == 0x10 || (symbol->kind & 31) == 0x11) {
                    binding = qualified->resolution->binding->next;
                    if (binding) {
                        member_class = (CppSymbol *)binding->target;
                        if ((member_class->kind & 31) == 15)
                            member_class = cpp_context_stack->items[((ClassReference *)member_class)->context_index];
                        parser_value.symbol->auxiliary.value =
                            (parser_value.symbol->auxiliary.value & 0xffff0000UL) |
                            (unsigned short)member_class->detail.class_info->type_index;
                    }
                    parser_value.symbol->cpp.parts.token = ((OverloadSet *)symbol)->functions->cpp.parts.token;
                }
            }
            break;
        case 118:
            qualified = right[-1].context;
            if (qualified) {
                conversion_declaration = right[0].declaration;
                parser_name = encode_type_name(conversion_declaration.type);
                qualified->resolution = lookup_qualified_member(qualified->resolution,
                    parser_name, conversion_declaration.type);
                if (!qualified->resolution)
                    parser_value.symbol = new_declarator(make_anonymous_identifier());
                else {
                    parser_value.symbol = new_declarator(parser_name);
                    parser_value.symbol->next = (CppSymbol *)qualified;
                    parser_value.symbol->cpp.parts.token = 0x9a;
                    parser_value.symbol->declaration_flags |= 0x4000;
                }
            }
            token_allocation_flags &= ~4;
            break;
        case 119:
            apply_conversion_declarator(&right[0].declaration, 0);
            parser_value = right[0];
            parser_value.bytes[0] &= 0x7f;
            break;
        case 120:
            apply_conversion_declarator(&right[-1].declaration, right[0].shape);
            parser_value = right[-1];
            parser_value.bytes[0] &= 0x7f;
            break;
        case 121:
            pending_function_qualifiers = 0;
            break;
        case 122:
            pending_function_qualifiers = right[0].shape->attributes;
            break;
        case 123:
            report_missing_declarator(240, &right[0].declaration);
            pending_function_qualifiers = 0;
            parser_recovery_count = 0;
            parser_lookahead = -1;
            break;
        case 124:
            report_syntax_error(290, token_spellings[189]);
            parser_value.symbol = right[-2].symbol;
            break;
        case 125:
            parser_value.symbol = right[0].symbol;
            break;
        case 126:
            parser_value.symbol = empty_parameter_declaration();
            POP_DECLARATION();
            declaration_kind = 1;
            begin_parameter_scope(0);
            break;
        case 127:
            ++suppress_implicit_member;
            ++declaration_mode;
            begin_parameter_scope(0);
            break;
        case 128:
            if (right[-1].symbol) {
                insert_current_scope_symbol(right[-1].symbol);
                parser_value.symbol = append_parameter_declaration(right[-2].symbol, right[-1].symbol);
            } else parser_value.symbol = right[-2].symbol;
            declaration_kind = 3;
            --suppress_implicit_member;
            --declaration_mode;
            POP_DECLARATION();
            break;
        case 129:
            ALLOCATE_PARSER_NODE(parser_value.symbol, sizeof(CppSymbol), declaration_mode != 0);
            parser_value.symbol->kind = 10;
            parser_value.symbol->name = name_ellipsis;
            parser_value.symbol->flags |= 8;
            parser_value.symbol->detail.parameter_next = parser_value.symbol;
            declaration_kind = 3;
            POP_DECLARATION();
            begin_parameter_scope(0);
            break;
        case 130:
            parser_value.symbol = append_parameter_declaration(0, right[0].symbol);
            break;
        case 131:
            parser_value.symbol = append_parameter_declaration(right[-2].symbol, right[0].symbol);
            break;
        case 132:
            if (parser_lookahead == -1) parser_lookahead = scan_token();
            if (parser_lookahead == 0x8a) {
                unget_token(0x8a);
                parser_lookahead = 0x88;
                report_syntax_error(235);
            }
            declaration_spec_flags[declaration_depth] |= 8;
            break;
        case 133:
            if (right[0].symbol && default_arguments[declaration_depth-1]) {
                token_allocation_flags |= 0x20;
                default_arguments[declaration_depth-1] = (DefaultArgument *)
                    append_expression((IrToken *)default_arguments[declaration_depth-1], 0, 2);
                token_allocation_flags &= ~0x20;
            }
            declaration_spec_flags[declaration_depth] &= ~2;
            break;
        case 134:
            unwind_qualified_scopes(1);
            if (right[-1].symbol && right[0].expression) {
                right[0].expression = validate_value_expression(right[0].expression);
                type = right[-1].symbol->type;
                if (!can_initialize(type, right[0].expression))
                    report_type_conversion(434, 0, 0x6a, right[0].expression->type, type);
                else {
                    check_parameter_default(right[0].expression, type);
                    right[0].expression = convert_initializer(1, type, right[0].expression);
                    right[0].expression = convert_default_argument(type, right[0].expression);
                }
                default_arguments[declaration_depth-1] =
                    (DefaultArgument *)save_default_argument(right[0].expression);
            }
            token_allocation_flags &= ~0x20;
            declaration_spec_flags[declaration_depth] &= ~2;
            break;
        case 135:
            ALLOCATE_PARSER_NODE(expression, sizeof(IrToken),
                (token_allocation_flags & 0x20000020UL) ? 0 : 2);
            expression->flags = 5;
            expression->kind = 0xc3;
            expression->value.pointer = right[0].context;
            *(unsigned *)&expression->length = right[0].words.high;
            default_arguments[declaration_depth-1] = (DefaultArgument *)append_expression(
                (IrToken *)default_arguments[declaration_depth-1], expression, 2);
            finish_default_argument_text();
            token_allocation_flags &= ~0x20;
            declaration_spec_flags[declaration_depth] &= ~2;
            break;
        case 136:
            if (parser_lookahead == 0x7f) {
                report_syntax_error(279);
                parser_lookahead = scan_token();
                synchronize_tokens(parser_lookahead, 0x85, 1);
                if (source_text_cursor) ++source_text_cursor;
                parser_lookahead = -1;
            } else
                parser_lookahead = synchronize_tokens((unsigned char)parser_lookahead, 0x88, 0x40, 1);
            parser_recovery_count = 0;
            token_allocation_flags &= ~0x20;
            declaration_spec_flags[declaration_depth] &= ~2;
            break;
        case 137:
            token_allocation_flags |= 0x20;
            mark_evaluation_boundary();
            break;
        case 139:
            insert_current_scope_symbol(right[0].symbol);
            break;
        case 140:
            parser_value.symbol = 0;
            parser_lookahead = scan_token();
            parser_lookahead = synchronize_tokens(parser_lookahead, 0x8a, 0x88, 0x7f, 1);
            unget_token((unsigned char)parser_lookahead);
            if (parser_lookahead == 0xa1) unwind_qualified_scopes(1);
            parser_recovery_count = 0;
            parser_lookahead = -1;
            finish_parameter_scope();
            break;
        case 141:
            parser_value.symbol = right[0].symbol;
            right[0].symbol->kind = 10;
            resolve_declaration(&right[-1].declaration, parser_value.symbol);
            --declarator_nesting;
            CLEAR_DECLARATION_FLAGS();
            break;
        case 142:
            parser_value.symbol = right[0].symbol;
            if (next_symbol_index >= 0x7fffffffU) fatal_error(55);
            if (next_symbol_index >= browser_symbol_limit && browser_filename) browser_symbol_rollover();
            parser_value.symbol->index = next_symbol_index++;
            parser_value.symbol->kind = 11;
            parser_value.symbol->name = name_formal;
            parser_value.symbol->references = 1;
            right[-1].bytes[0] &= 0x7f;
            parser_value.symbol->storage[0] = right[-1].bytes[0];
            parser_value.symbol->storage[1] = right[-1].bytes[1];
            parser_value.symbol->storage[2] = right[-1].bytes[2];
            normalize_symbol_type(right[-1].declaration.type, parser_value.symbol);
            --declarator_nesting;
            CLEAR_DECLARATION_FLAGS();
            break;
        case 143:
            parser_value.shape = new_type_shape(0, 0, 0);
            break;
        case 145:
        case 181:
            parser_value.shape = new_type_shape(0, right[0].modifier, &based_symbol);
            break;
        case 146:
        case 182:
            parser_value.shape = merge_type_shape_prefix(right[-1].shape,
                new_type_shape(0, right[0].modifier, &based_symbol));
            break;
        case 147:
            READ_DECLARATION_CODE(parser_value.modifier, right[0].bytes[0]);
            if (parser_value.modifier == 0x70 && !(token_allocation_flags & 0x400)) {
                report_syntax_error(61, token_spellings[204]);
                parser_value.modifier = 0x60;
            }
            break;
        case 148:
        case 149:
        case 183:
        case 184:
        case 185:
            READ_DECLARATION_CODE(parser_value.modifier, right[0].bytes[0]);
            break;
        case 151:
            if (right[-1].modifier) token_allocation_flags |= 0x800;
            parser_value.modifier = right[-1].modifier;
            POP_DECLARATION();
            cpp_parser_flags &= ~1;
            if (flags & 0x40) CLEAR_DECLARATION_FLAGS();
            break;
        case 153:
            parser_value.modifier = 5;
            based_symbol = right[0].symbol;
            break;
        case 155:
            parser_value.modifier = 5;
            based_symbol = lookup_based_symbol(right[0].name);
            if (!based_symbol) parser_value.modifier = 2;
            break;
        case 156:
            if (!validate_based_self()) parser_value.modifier = 0;
            break;
        case 157:
            parser_value.modifier = 4;
            break;
        case 158:
            parser_recovery_count = 0;
            parser_lookahead = -1;
            parser_value.modifier = 0;
            break;
        case 159:
            parser_value.modifier = validate_bit_field_bound(right[-2].declaration.type->flags,
                right[0].name);
            POP_DECLARATION();
            break;
        case 160:
            parser_value.modifier = validate_extended_bound(right[-3].declaration.type->flags,
                right[0].name);
            POP_DECLARATION();
            break;
        case 161:
            parser_value.modifier = validate_integer_bit_field_type(right[-2].declaration.type->flags);
            POP_DECLARATION();
            break;
        case 162:
            ++declarator_nesting;
            break;
        case 164:
            parser_value.symbol = new_abstract_declarator(0);
            break;
        case 165:
            parser_value.symbol = new_abstract_declarator(right[0].shape);
            break;
        case 166:
            finish_parameter_scope();
            parser_value.symbol = build_function_declarator(new_abstract_declarator(0), right[0].symbol);
            break;
        case 167:
            finish_parameter_scope();
            parser_value.symbol = build_function_declarator(new_abstract_declarator(right[-1].shape), right[0].symbol);
            break;
        case 168:
            parser_value.symbol = right[0].symbol;
            right[-2].shape->flags = 1;
            attach_type_shape(parser_value.symbol, right[-2].shape);
            break;
        case 169:
            parser_value.symbol = right[0].symbol;
            right[-2].shape->flags = 0x40;
            attach_type_shape(parser_value.symbol, right[-2].shape);
            break;
        case 170:
            parser_value.symbol = right[0].symbol;
            resolve_member_pointer_class(&member_class, right[-1].context);
            if (member_class) {
                right[-2].shape->flags = 0x10;
                right[-2].shape->data.tag_symbol = member_class;
            }
            attach_type_shape(parser_value.symbol, right[-2].shape);
            break;
        case 171:
            add_array_declarator(right[-3].symbol, right[-1].expression);
            break;
        case 172:
            finish_parameter_scope();
            parser_value.symbol = build_function_declarator(right[-1].symbol, right[0].symbol);
            break;
        case 174:
            parser_value.symbol = right[-1].symbol;
            POP_DECLARATION();
            break;
        case 176:
            shape = parser_value.shape;
            while (shape->related) shape = shape->related;
            shape->related = right[0].shape;
            break;
        case 177:
            parser_value.shape = new_type_shape(1, 0, &based_symbol);
            break;
        case 178:
            parser_value.shape = new_type_shape(0x40, 0, &based_symbol);
            break;
        case 179:
            parser_value.shape->flags = 1;
            break;
        case 180:
            parser_value.shape->flags = 0x40;
            break;
        case 186:
            token_allocation_flags |= 0x800;
            parser_value.modifier = right[-1].modifier;
            POP_DECLARATION();
            break;
        case 187:
            parser_value.symbol = 0;
            break;
        case 188:
        case 189:
            ALLOCATE_PARSER_NODE(parser_value.symbol, sizeof(CppSymbol), declaration_mode != 0);
            parser_value.symbol->kind = 10;
            parser_value.symbol->name = name_ellipsis;
            parser_value.symbol->flags |= 8;
            break;
