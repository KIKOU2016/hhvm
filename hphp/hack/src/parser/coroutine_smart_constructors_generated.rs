/**
 * Copyright (c) 2016, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the "hack" directory of this source tree. An additional
 * directory.
 *
 **
 *
 * THIS FILE IS @generated; DO NOT EDIT IT
 * To regenerate this file, run
 *
 *   buck run //hphp/hack/src:generate_full_fidelity
 *
 **
 *
 */
use crate::coroutine_smart_constructors::*;
use crate::lexable_token::LexableToken;
use crate::parser_env::ParserEnv;
use crate::smart_constructors::SmartConstructors;
use crate::source_text::SourceText;
use crate::syntax::*;
use crate::syntax_smart_constructors::{StateType, SyntaxSmartConstructors};

impl<'src, S, T, Token, Value> SmartConstructors<'src, T>
    for CoroutineSmartConstructors<'src, S, T>
where
    Token: LexableToken,
    Value: SyntaxValueType<Token> + SyntaxValueWithKind,
    S: SyntaxType<T, Token=Token, Value=Value>,
    T: StateType<'src, S> + CoroutineStateType,
{
    type Token = Token;
    type R = S;

    fn new(env: &ParserEnv, src: &SourceText<'src>) -> Self {
        <Self as SyntaxSmartConstructors<'src, Self::R, T>>::new(env, src)
    }

    fn state_mut(&mut self) -> &mut T {
        &mut self.state
    }

    fn make_missing(&mut self, offset: usize) -> Self::R {
       <Self as SyntaxSmartConstructors<'src, Self::R, T>>::make_missing(self, offset)
    }

    fn make_token(&mut self, offset: Self::Token) -> Self::R {
       <Self as SyntaxSmartConstructors<'src, Self::R, T>>::make_token(self, offset)
    }

    fn make_list(&mut self, lst: Vec<Self::R>, offset: usize) -> Self::R {
        <Self as SyntaxSmartConstructors<'src, Self::R, T>>::make_list(self, lst, offset)
    }

    fn make_end_of_file(&mut self, arg0: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_end_of_file(self, arg0)
    }

    fn make_script(&mut self, arg0: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_script(self, arg0)
    }

    fn make_qualified_name(&mut self, arg0: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_qualified_name(self, arg0)
    }

    fn make_simple_type_specifier(&mut self, arg0: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_simple_type_specifier(self, arg0)
    }

    fn make_literal_expression(&mut self, arg0: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_literal_expression(self, arg0)
    }

    fn make_prefixed_string_expression(&mut self, arg0: Self::R, arg1: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_prefixed_string_expression(self, arg0, arg1)
    }

    fn make_variable_expression(&mut self, arg0: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_variable_expression(self, arg0)
    }

    fn make_pipe_variable_expression(&mut self, arg0: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_pipe_variable_expression(self, arg0)
    }

    fn make_file_attribute_specification(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R, arg4: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_file_attribute_specification(self, arg0, arg1, arg2, arg3, arg4)
    }

    fn make_enum_declaration(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R, arg4: Self::R, arg5: Self::R, arg6: Self::R, arg7: Self::R, arg8: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_enum_declaration(self, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)
    }

    fn make_enumerator(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_enumerator(self, arg0, arg1, arg2, arg3)
    }

    fn make_record_declaration(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R, arg4: Self::R, arg5: Self::R, arg6: Self::R, arg7: Self::R, arg8: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_record_declaration(self, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)
    }

    fn make_record_field(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R, arg4: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_record_field(self, arg0, arg1, arg2, arg3, arg4)
    }

    fn make_alias_declaration(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R, arg4: Self::R, arg5: Self::R, arg6: Self::R, arg7: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_alias_declaration(self, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7)
    }

    fn make_property_declaration(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R, arg4: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_property_declaration(self, arg0, arg1, arg2, arg3, arg4)
    }

    fn make_property_declarator(&mut self, arg0: Self::R, arg1: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_property_declarator(self, arg0, arg1)
    }

    fn make_namespace_declaration(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_namespace_declaration(self, arg0, arg1, arg2)
    }

    fn make_namespace_body(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_namespace_body(self, arg0, arg1, arg2)
    }

    fn make_namespace_empty_body(&mut self, arg0: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_namespace_empty_body(self, arg0)
    }

    fn make_namespace_use_declaration(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_namespace_use_declaration(self, arg0, arg1, arg2, arg3)
    }

    fn make_namespace_group_use_declaration(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R, arg4: Self::R, arg5: Self::R, arg6: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_namespace_group_use_declaration(self, arg0, arg1, arg2, arg3, arg4, arg5, arg6)
    }

    fn make_namespace_use_clause(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_namespace_use_clause(self, arg0, arg1, arg2, arg3)
    }

    fn make_function_declaration(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_function_declaration(self, arg0, arg1, arg2)
    }

    fn make_function_declaration_header(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R, arg4: Self::R, arg5: Self::R, arg6: Self::R, arg7: Self::R, arg8: Self::R, arg9: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_function_declaration_header(self, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9)
    }

    fn make_where_clause(&mut self, arg0: Self::R, arg1: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_where_clause(self, arg0, arg1)
    }

    fn make_where_constraint(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_where_constraint(self, arg0, arg1, arg2)
    }

    fn make_methodish_declaration(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_methodish_declaration(self, arg0, arg1, arg2, arg3)
    }

    fn make_methodish_trait_resolution(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R, arg4: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_methodish_trait_resolution(self, arg0, arg1, arg2, arg3, arg4)
    }

    fn make_classish_declaration(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R, arg4: Self::R, arg5: Self::R, arg6: Self::R, arg7: Self::R, arg8: Self::R, arg9: Self::R, arg10: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_classish_declaration(self, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10)
    }

    fn make_classish_body(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_classish_body(self, arg0, arg1, arg2)
    }

    fn make_trait_use_precedence_item(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_trait_use_precedence_item(self, arg0, arg1, arg2)
    }

    fn make_trait_use_alias_item(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_trait_use_alias_item(self, arg0, arg1, arg2, arg3)
    }

    fn make_trait_use_conflict_resolution(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R, arg4: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_trait_use_conflict_resolution(self, arg0, arg1, arg2, arg3, arg4)
    }

    fn make_trait_use(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_trait_use(self, arg0, arg1, arg2)
    }

    fn make_require_clause(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_require_clause(self, arg0, arg1, arg2, arg3)
    }

    fn make_const_declaration(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R, arg4: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_const_declaration(self, arg0, arg1, arg2, arg3, arg4)
    }

    fn make_constant_declarator(&mut self, arg0: Self::R, arg1: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_constant_declarator(self, arg0, arg1)
    }

    fn make_type_const_declaration(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R, arg4: Self::R, arg5: Self::R, arg6: Self::R, arg7: Self::R, arg8: Self::R, arg9: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_type_const_declaration(self, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9)
    }

    fn make_decorated_expression(&mut self, arg0: Self::R, arg1: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_decorated_expression(self, arg0, arg1)
    }

    fn make_parameter_declaration(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R, arg4: Self::R, arg5: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_parameter_declaration(self, arg0, arg1, arg2, arg3, arg4, arg5)
    }

    fn make_variadic_parameter(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_variadic_parameter(self, arg0, arg1, arg2)
    }

    fn make_old_attribute_specification(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_old_attribute_specification(self, arg0, arg1, arg2)
    }

    fn make_attribute_specification(&mut self, arg0: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_attribute_specification(self, arg0)
    }

    fn make_attribute(&mut self, arg0: Self::R, arg1: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_attribute(self, arg0, arg1)
    }

    fn make_inclusion_expression(&mut self, arg0: Self::R, arg1: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_inclusion_expression(self, arg0, arg1)
    }

    fn make_inclusion_directive(&mut self, arg0: Self::R, arg1: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_inclusion_directive(self, arg0, arg1)
    }

    fn make_compound_statement(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_compound_statement(self, arg0, arg1, arg2)
    }

    fn make_expression_statement(&mut self, arg0: Self::R, arg1: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_expression_statement(self, arg0, arg1)
    }

    fn make_markup_section(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_markup_section(self, arg0, arg1, arg2, arg3)
    }

    fn make_markup_suffix(&mut self, arg0: Self::R, arg1: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_markup_suffix(self, arg0, arg1)
    }

    fn make_unset_statement(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R, arg4: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_unset_statement(self, arg0, arg1, arg2, arg3, arg4)
    }

    fn make_let_statement(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R, arg4: Self::R, arg5: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_let_statement(self, arg0, arg1, arg2, arg3, arg4, arg5)
    }

    fn make_using_statement_block_scoped(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R, arg4: Self::R, arg5: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_using_statement_block_scoped(self, arg0, arg1, arg2, arg3, arg4, arg5)
    }

    fn make_using_statement_function_scoped(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_using_statement_function_scoped(self, arg0, arg1, arg2, arg3)
    }

    fn make_while_statement(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R, arg4: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_while_statement(self, arg0, arg1, arg2, arg3, arg4)
    }

    fn make_if_statement(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R, arg4: Self::R, arg5: Self::R, arg6: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_if_statement(self, arg0, arg1, arg2, arg3, arg4, arg5, arg6)
    }

    fn make_elseif_clause(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R, arg4: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_elseif_clause(self, arg0, arg1, arg2, arg3, arg4)
    }

    fn make_else_clause(&mut self, arg0: Self::R, arg1: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_else_clause(self, arg0, arg1)
    }

    fn make_try_statement(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_try_statement(self, arg0, arg1, arg2, arg3)
    }

    fn make_catch_clause(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R, arg4: Self::R, arg5: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_catch_clause(self, arg0, arg1, arg2, arg3, arg4, arg5)
    }

    fn make_finally_clause(&mut self, arg0: Self::R, arg1: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_finally_clause(self, arg0, arg1)
    }

    fn make_do_statement(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R, arg4: Self::R, arg5: Self::R, arg6: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_do_statement(self, arg0, arg1, arg2, arg3, arg4, arg5, arg6)
    }

    fn make_for_statement(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R, arg4: Self::R, arg5: Self::R, arg6: Self::R, arg7: Self::R, arg8: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_for_statement(self, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)
    }

    fn make_foreach_statement(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R, arg4: Self::R, arg5: Self::R, arg6: Self::R, arg7: Self::R, arg8: Self::R, arg9: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_foreach_statement(self, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9)
    }

    fn make_switch_statement(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R, arg4: Self::R, arg5: Self::R, arg6: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_switch_statement(self, arg0, arg1, arg2, arg3, arg4, arg5, arg6)
    }

    fn make_switch_section(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_switch_section(self, arg0, arg1, arg2)
    }

    fn make_switch_fallthrough(&mut self, arg0: Self::R, arg1: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_switch_fallthrough(self, arg0, arg1)
    }

    fn make_case_label(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_case_label(self, arg0, arg1, arg2)
    }

    fn make_default_label(&mut self, arg0: Self::R, arg1: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_default_label(self, arg0, arg1)
    }

    fn make_return_statement(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_return_statement(self, arg0, arg1, arg2)
    }

    fn make_goto_label(&mut self, arg0: Self::R, arg1: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_goto_label(self, arg0, arg1)
    }

    fn make_goto_statement(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_goto_statement(self, arg0, arg1, arg2)
    }

    fn make_throw_statement(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_throw_statement(self, arg0, arg1, arg2)
    }

    fn make_break_statement(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_break_statement(self, arg0, arg1, arg2)
    }

    fn make_continue_statement(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_continue_statement(self, arg0, arg1, arg2)
    }

    fn make_echo_statement(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_echo_statement(self, arg0, arg1, arg2)
    }

    fn make_concurrent_statement(&mut self, arg0: Self::R, arg1: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_concurrent_statement(self, arg0, arg1)
    }

    fn make_simple_initializer(&mut self, arg0: Self::R, arg1: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_simple_initializer(self, arg0, arg1)
    }

    fn make_anonymous_class(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R, arg4: Self::R, arg5: Self::R, arg6: Self::R, arg7: Self::R, arg8: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_anonymous_class(self, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)
    }

    fn make_anonymous_function(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R, arg4: Self::R, arg5: Self::R, arg6: Self::R, arg7: Self::R, arg8: Self::R, arg9: Self::R, arg10: Self::R, arg11: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_anonymous_function(self, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11)
    }

    fn make_anonymous_function_use_clause(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_anonymous_function_use_clause(self, arg0, arg1, arg2, arg3)
    }

    fn make_lambda_expression(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R, arg4: Self::R, arg5: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_lambda_expression(self, arg0, arg1, arg2, arg3, arg4, arg5)
    }

    fn make_lambda_signature(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R, arg4: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_lambda_signature(self, arg0, arg1, arg2, arg3, arg4)
    }

    fn make_cast_expression(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_cast_expression(self, arg0, arg1, arg2, arg3)
    }

    fn make_scope_resolution_expression(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_scope_resolution_expression(self, arg0, arg1, arg2)
    }

    fn make_member_selection_expression(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_member_selection_expression(self, arg0, arg1, arg2)
    }

    fn make_safe_member_selection_expression(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_safe_member_selection_expression(self, arg0, arg1, arg2)
    }

    fn make_embedded_member_selection_expression(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_embedded_member_selection_expression(self, arg0, arg1, arg2)
    }

    fn make_yield_expression(&mut self, arg0: Self::R, arg1: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_yield_expression(self, arg0, arg1)
    }

    fn make_yield_from_expression(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_yield_from_expression(self, arg0, arg1, arg2)
    }

    fn make_prefix_unary_expression(&mut self, arg0: Self::R, arg1: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_prefix_unary_expression(self, arg0, arg1)
    }

    fn make_postfix_unary_expression(&mut self, arg0: Self::R, arg1: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_postfix_unary_expression(self, arg0, arg1)
    }

    fn make_binary_expression(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_binary_expression(self, arg0, arg1, arg2)
    }

    fn make_is_expression(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_is_expression(self, arg0, arg1, arg2)
    }

    fn make_as_expression(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_as_expression(self, arg0, arg1, arg2)
    }

    fn make_nullable_as_expression(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_nullable_as_expression(self, arg0, arg1, arg2)
    }

    fn make_conditional_expression(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R, arg4: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_conditional_expression(self, arg0, arg1, arg2, arg3, arg4)
    }

    fn make_eval_expression(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_eval_expression(self, arg0, arg1, arg2, arg3)
    }

    fn make_define_expression(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_define_expression(self, arg0, arg1, arg2, arg3)
    }

    fn make_halt_compiler_expression(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_halt_compiler_expression(self, arg0, arg1, arg2, arg3)
    }

    fn make_isset_expression(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_isset_expression(self, arg0, arg1, arg2, arg3)
    }

    fn make_function_call_expression(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R, arg4: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_function_call_expression(self, arg0, arg1, arg2, arg3, arg4)
    }

    fn make_parenthesized_expression(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_parenthesized_expression(self, arg0, arg1, arg2)
    }

    fn make_braced_expression(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_braced_expression(self, arg0, arg1, arg2)
    }

    fn make_embedded_braced_expression(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_embedded_braced_expression(self, arg0, arg1, arg2)
    }

    fn make_list_expression(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_list_expression(self, arg0, arg1, arg2, arg3)
    }

    fn make_collection_literal_expression(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_collection_literal_expression(self, arg0, arg1, arg2, arg3)
    }

    fn make_object_creation_expression(&mut self, arg0: Self::R, arg1: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_object_creation_expression(self, arg0, arg1)
    }

    fn make_constructor_call(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_constructor_call(self, arg0, arg1, arg2, arg3)
    }

    fn make_record_creation_expression(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R, arg4: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_record_creation_expression(self, arg0, arg1, arg2, arg3, arg4)
    }

    fn make_array_creation_expression(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_array_creation_expression(self, arg0, arg1, arg2)
    }

    fn make_array_intrinsic_expression(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_array_intrinsic_expression(self, arg0, arg1, arg2, arg3)
    }

    fn make_darray_intrinsic_expression(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R, arg4: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_darray_intrinsic_expression(self, arg0, arg1, arg2, arg3, arg4)
    }

    fn make_dictionary_intrinsic_expression(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R, arg4: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_dictionary_intrinsic_expression(self, arg0, arg1, arg2, arg3, arg4)
    }

    fn make_keyset_intrinsic_expression(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R, arg4: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_keyset_intrinsic_expression(self, arg0, arg1, arg2, arg3, arg4)
    }

    fn make_varray_intrinsic_expression(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R, arg4: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_varray_intrinsic_expression(self, arg0, arg1, arg2, arg3, arg4)
    }

    fn make_vector_intrinsic_expression(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R, arg4: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_vector_intrinsic_expression(self, arg0, arg1, arg2, arg3, arg4)
    }

    fn make_element_initializer(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_element_initializer(self, arg0, arg1, arg2)
    }

    fn make_subscript_expression(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_subscript_expression(self, arg0, arg1, arg2, arg3)
    }

    fn make_embedded_subscript_expression(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_embedded_subscript_expression(self, arg0, arg1, arg2, arg3)
    }

    fn make_awaitable_creation_expression(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_awaitable_creation_expression(self, arg0, arg1, arg2, arg3)
    }

    fn make_xhp_children_declaration(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_xhp_children_declaration(self, arg0, arg1, arg2)
    }

    fn make_xhp_children_parenthesized_list(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_xhp_children_parenthesized_list(self, arg0, arg1, arg2)
    }

    fn make_xhp_category_declaration(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_xhp_category_declaration(self, arg0, arg1, arg2)
    }

    fn make_xhp_enum_type(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R, arg4: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_xhp_enum_type(self, arg0, arg1, arg2, arg3, arg4)
    }

    fn make_xhp_lateinit(&mut self, arg0: Self::R, arg1: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_xhp_lateinit(self, arg0, arg1)
    }

    fn make_xhp_required(&mut self, arg0: Self::R, arg1: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_xhp_required(self, arg0, arg1)
    }

    fn make_xhp_class_attribute_declaration(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_xhp_class_attribute_declaration(self, arg0, arg1, arg2)
    }

    fn make_xhp_class_attribute(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_xhp_class_attribute(self, arg0, arg1, arg2, arg3)
    }

    fn make_xhp_simple_class_attribute(&mut self, arg0: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_xhp_simple_class_attribute(self, arg0)
    }

    fn make_xhp_simple_attribute(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_xhp_simple_attribute(self, arg0, arg1, arg2)
    }

    fn make_xhp_spread_attribute(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_xhp_spread_attribute(self, arg0, arg1, arg2, arg3)
    }

    fn make_xhp_open(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_xhp_open(self, arg0, arg1, arg2, arg3)
    }

    fn make_xhp_expression(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_xhp_expression(self, arg0, arg1, arg2)
    }

    fn make_xhp_close(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_xhp_close(self, arg0, arg1, arg2)
    }

    fn make_type_constant(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_type_constant(self, arg0, arg1, arg2)
    }

    fn make_vector_type_specifier(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R, arg4: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_vector_type_specifier(self, arg0, arg1, arg2, arg3, arg4)
    }

    fn make_keyset_type_specifier(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R, arg4: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_keyset_type_specifier(self, arg0, arg1, arg2, arg3, arg4)
    }

    fn make_tuple_type_explicit_specifier(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_tuple_type_explicit_specifier(self, arg0, arg1, arg2, arg3)
    }

    fn make_varray_type_specifier(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R, arg4: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_varray_type_specifier(self, arg0, arg1, arg2, arg3, arg4)
    }

    fn make_vector_array_type_specifier(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_vector_array_type_specifier(self, arg0, arg1, arg2, arg3)
    }

    fn make_type_parameter(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R, arg4: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_type_parameter(self, arg0, arg1, arg2, arg3, arg4)
    }

    fn make_type_constraint(&mut self, arg0: Self::R, arg1: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_type_constraint(self, arg0, arg1)
    }

    fn make_darray_type_specifier(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R, arg4: Self::R, arg5: Self::R, arg6: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_darray_type_specifier(self, arg0, arg1, arg2, arg3, arg4, arg5, arg6)
    }

    fn make_map_array_type_specifier(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R, arg4: Self::R, arg5: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_map_array_type_specifier(self, arg0, arg1, arg2, arg3, arg4, arg5)
    }

    fn make_dictionary_type_specifier(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_dictionary_type_specifier(self, arg0, arg1, arg2, arg3)
    }

    fn make_closure_type_specifier(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R, arg4: Self::R, arg5: Self::R, arg6: Self::R, arg7: Self::R, arg8: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_closure_type_specifier(self, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)
    }

    fn make_closure_parameter_type_specifier(&mut self, arg0: Self::R, arg1: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_closure_parameter_type_specifier(self, arg0, arg1)
    }

    fn make_classname_type_specifier(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R, arg4: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_classname_type_specifier(self, arg0, arg1, arg2, arg3, arg4)
    }

    fn make_field_specifier(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_field_specifier(self, arg0, arg1, arg2, arg3)
    }

    fn make_field_initializer(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_field_initializer(self, arg0, arg1, arg2)
    }

    fn make_shape_type_specifier(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R, arg4: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_shape_type_specifier(self, arg0, arg1, arg2, arg3, arg4)
    }

    fn make_shape_expression(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_shape_expression(self, arg0, arg1, arg2, arg3)
    }

    fn make_tuple_expression(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_tuple_expression(self, arg0, arg1, arg2, arg3)
    }

    fn make_generic_type_specifier(&mut self, arg0: Self::R, arg1: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_generic_type_specifier(self, arg0, arg1)
    }

    fn make_nullable_type_specifier(&mut self, arg0: Self::R, arg1: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_nullable_type_specifier(self, arg0, arg1)
    }

    fn make_like_type_specifier(&mut self, arg0: Self::R, arg1: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_like_type_specifier(self, arg0, arg1)
    }

    fn make_soft_type_specifier(&mut self, arg0: Self::R, arg1: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_soft_type_specifier(self, arg0, arg1)
    }

    fn make_attributized_specifier(&mut self, arg0: Self::R, arg1: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_attributized_specifier(self, arg0, arg1)
    }

    fn make_reified_type_argument(&mut self, arg0: Self::R, arg1: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_reified_type_argument(self, arg0, arg1)
    }

    fn make_type_arguments(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_type_arguments(self, arg0, arg1, arg2)
    }

    fn make_type_parameters(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_type_parameters(self, arg0, arg1, arg2)
    }

    fn make_tuple_type_specifier(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_tuple_type_specifier(self, arg0, arg1, arg2)
    }

    fn make_error(&mut self, arg0: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_error(self, arg0)
    }

    fn make_list_item(&mut self, arg0: Self::R, arg1: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_list_item(self, arg0, arg1)
    }

    fn make_pocket_atom_expression(&mut self, arg0: Self::R, arg1: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_pocket_atom_expression(self, arg0, arg1)
    }

    fn make_pocket_identifier_expression(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R, arg4: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_pocket_identifier_expression(self, arg0, arg1, arg2, arg3, arg4)
    }

    fn make_pocket_atom_mapping_declaration(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R, arg4: Self::R, arg5: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_pocket_atom_mapping_declaration(self, arg0, arg1, arg2, arg3, arg4, arg5)
    }

    fn make_pocket_enum_declaration(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R, arg4: Self::R, arg5: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_pocket_enum_declaration(self, arg0, arg1, arg2, arg3, arg4, arg5)
    }

    fn make_pocket_field_type_expr_declaration(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_pocket_field_type_expr_declaration(self, arg0, arg1, arg2, arg3)
    }

    fn make_pocket_field_type_declaration(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_pocket_field_type_declaration(self, arg0, arg1, arg2, arg3)
    }

    fn make_pocket_mapping_id_declaration(&mut self, arg0: Self::R, arg1: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_pocket_mapping_id_declaration(self, arg0, arg1)
    }

    fn make_pocket_mapping_type_declaration(&mut self, arg0: Self::R, arg1: Self::R, arg2: Self::R, arg3: Self::R) -> Self::R {
        <Self as SyntaxSmartConstructors<Self::R, T>>::make_pocket_mapping_type_declaration(self, arg0, arg1, arg2, arg3)
    }

}
