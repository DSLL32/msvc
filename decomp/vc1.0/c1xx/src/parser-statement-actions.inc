        case 190:
            if (!options.prototypes_only && !(token_allocation_flags & 0x01000000)) {
                if (parser_lookahead != -1) {
                    unget_token((unsigned char)parser_lookahead);
                    if (parser_lookahead == 0xa1) unwind_qualified_scopes(1);
                    parser_lookahead = -1;
                }
                begin_compound_statement();
                CLEAR_DECLARATION_FLAGS();
            } else skip_balanced_brace_body();
            break;
        case 191:
            if (!options.prototypes_only && !(token_allocation_flags & 0x01000000))
                end_compound_statement();
            else token_allocation_flags &= ~0x01000000;
            break;
        case 195:
            declaration_depth = 0;
            token_allocation_flags &= ~1;
            declaration_spec_flags[0] &= ~3;
            deferred_declarations[0] = 0;
            if (parser_lookahead != -1) {
                unget_token((unsigned char)parser_lookahead);
                if (parser_lookahead == 0xa1) unwind_qualified_scopes(1);
                parser_lookahead = -1;
            }
            break;
        case 196:
        case 247:
            if (right[-1].expression) {
                right[-1].expression = validate_value_expression(right[-1].expression);
                emit_expression_statement(prepare_expression(right[-1].expression, 2));
            }
            RESET_EXPRESSION_ARENA();
            break;
        case 199:
            if (parser_lookahead == 0xa1) {
                unget_token((unsigned char)parser_lookahead);
                unwind_qualified_scopes(1);
                parser_lookahead = -1;
            }
            finish_implicit_statement_scope(right[-1].expression, 0);
            emit_label(right[-1].expression);
            end_if_statement(1);
            break;
        case 200:
            finish_implicit_statement_scope(right[-1].expression, 0);
            emit_label(right[-1].expression);
            end_else_statement();
            break;
        case 201:
            begin_while_condition();
            declaration_spec_flags[declaration_depth] |= 3;
            break;
        case 202:
            if (normalized_Ol && options.configuration.option_qc) begin_qc_control(0);
            right[-1].expression = validate_value_expression(right[-1].expression);
            right[-1].expression = boolean_expression(right[-1].expression, 0);
            emit_conditional_branch(0, right[-1].expression, 0);
            POP_DECLARATION();
            declaration_spec_flags[declaration_depth] &= ~1;
            begin_while_body();
            right[-4].words.low = begin_implicit_statement_scope(0);
            break;
        case 203:
            finish_implicit_statement_scope(0, right[-6].words.low);
            end_while_statement();
            finish_conditional_branch();
            if (normalized_Ol && options.configuration.option_qc) end_qc_control();
            break;
        case 205:
            begin_for_condition();
            declaration_spec_flags[declaration_depth] |= 2;
            break;
        case 206:
            right[-1].expression = boolean_expression(right[-1].expression, 0);
            begin_for_increment();
            break;
        case 207:
            if (normalized_Ol && options.configuration.option_qc) begin_qc_control(0);
            emit_conditional_branch(0, right[-5].expression, right[-2].expression);
            if (right[-1].bytes[0]) report_syntax_error(59, token_spellings[0x8a]);
            POP_DECLARATION();
            CLEAR_DECLARATION_FLAGS();
            begin_for_body();
            right[-10].words.low = begin_implicit_statement_scope(0);
            break;
        case 208:
            finish_implicit_statement_scope(0, right[-12].words.low);
            end_for_statement();
            finish_conditional_branch();
            if (normalized_Ol && options.configuration.option_qc) end_qc_control();
            break;
        case 209:
            if (normalized_Ol && options.configuration.option_qc) begin_qc_control(0);
            begin_do_statement();
            prepare_do_body();
            CLEAR_DECLARATION_FLAGS();
            right[0].words.low = begin_implicit_statement_scope(0);
            break;
        case 210:
            finish_implicit_statement_scope(0, right[-4].words.low);
            begin_do_condition();
            declaration_spec_flags[declaration_depth] |= 3;
            break;
        case 211:
            emit_label(continue_labels->label);
            if (right[-2].expression) {
                right[-2].expression = validate_value_expression(right[-2].expression);
                right[-2].expression = boolean_expression(right[-2].expression, 0);
                emit_condition_to_label(right[-2].expression, control_labels->label, 1);
            }
            record_line_information(1);
            emit_label(break_labels->label);
            control_labels = control_labels->next;
            continue_labels = continue_labels->next;
            break_labels = break_labels->next;
            label_stack_count -= 3;
            if (!label_stack_count) release_arena(3);
            RESET_EXPRESSION_ARENA();
            end_do_statement();
            if (normalized_Ol && options.configuration.option_qc) end_qc_control();
            POP_DECLARATION();
            break;
        case 212:
            begin_switch_condition();
            declaration_spec_flags[declaration_depth] |= 3;
            break;
        case 213:
            if (normalized_Ol && options.configuration.option_qc) begin_qc_control(0x10);
            switch_condition = validate_value_expression(right[-1].expression);
            begin_switch_body(switch_condition);
            if (options.configuration.option_qc) current_qc_context = 1;
            POP_DECLARATION();
            CLEAR_DECLARATION_FLAGS();
            right[-4].words.low = begin_implicit_statement_scope(0);
            break;
        case 214:
            finish_implicit_statement_scope(0, right[-6].words.low);
            record_line_information(1);
            write_ir_record(0x3a, break_labels->label);
            write_switch_dispatch();
            end_switch_statement();
            control_labels = control_labels->next;
            break_labels = break_labels->next;
            default_labels = default_labels->next;
            switch_labels = switch_labels->next;
            label_stack_count -= 4;
            if (!label_stack_count) release_arena(3);
            if (options.configuration.option_qc) {
                current_qc_context = 0;
                if (normalized_Ol) end_qc_control();
            }
            break;
        case 215:
            emit_seh_leave();
            break;
        case 216:
            if (!break_labels) report_syntax_error(43);
            else {
                record_break_statement();
                if (normalized_Ol && options.configuration.option_qc) record_qc_jump(1);
                write_ir_record(0x3a, break_labels->label);
            }
            break;
        case 217:
            if (!continue_labels) report_syntax_error(44);
            else {
                record_continue_statement();
                if (normalized_Ol && options.configuration.option_qc) record_qc_jump(2);
                write_ir_record(0x3a, continue_labels->label);
            }
            break;
        case 218:
            begin_return_expression();
            emit_return_statement(0, 1);
            break;
        case 219:
            begin_return_expression();
            break;
        case 220:
            right[-1].expression = validate_value_expression(right[-1].expression);
            emit_return_statement(right[-1].expression, 2);
            RESET_EXPRESSION_ARENA();
            break;
        case 221:
            if (normalized_Ol && options.configuration.option_qc) record_qc_jump(0);
            goto_name = right[-1].name;
            symbol = find_label(goto_name);
            i = symbol != 0;
            if (!symbol) {
                symbol = new_label(goto_name);
                register_label(symbol);
            }
            record_goto(symbol, i);
            write_ir_record(0x3a, symbol_reference(symbol));
            RESET_EXPRESSION_ARENA();
            break;
        case 222:
            emit_inline_assembly();
            break;
        case 223:
            end_seh_except();
            break;
        case 224:
            begin_seh_finally();
            break;
        case 225:
            end_seh_finally();
            break;
        case 227:
            report_syntax_error(181);
            break;
        case 229:
            parser_recovery_count = 0;
            parser_lookahead = -1;
            break;
        case 230:
            report_syntax_error(290, token_spellings[190]);
            break;
        case 235:
            report_syntax_error(290, token_spellings[187]);
            break;
        case 240:
            begin_seh_try();
            break;
        case 241:
            end_seh_try();
            break;
        case 242:
            begin_seh_filter();
            break;
        case 243:
            emit_seh_filter(right[-1].expression);
            end_seh_filter();
            POP_DECLARATION();
            break;
        case 244:
            parser_value.bytes[0] = 0;
            break;
