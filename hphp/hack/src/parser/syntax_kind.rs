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
use crate::token_kind::TokenKind;

#[derive(Debug, Copy, Clone)]
pub enum SyntaxKind {
    Missing,
    Token(TokenKind),
    SyntaxList,
    EndOfFile,
    Script,
    QualifiedName,
    SimpleTypeSpecifier,
    LiteralExpression,
    PrefixedStringExpression,
    VariableExpression,
    PipeVariableExpression,
    FileAttributeSpecification,
    EnumDeclaration,
    Enumerator,
    RecordDeclaration,
    RecordField,
    AliasDeclaration,
    PropertyDeclaration,
    PropertyDeclarator,
    NamespaceDeclaration,
    NamespaceBody,
    NamespaceEmptyBody,
    NamespaceUseDeclaration,
    NamespaceGroupUseDeclaration,
    NamespaceUseClause,
    FunctionDeclaration,
    FunctionDeclarationHeader,
    WhereClause,
    WhereConstraint,
    MethodishDeclaration,
    MethodishTraitResolution,
    ClassishDeclaration,
    ClassishBody,
    TraitUsePrecedenceItem,
    TraitUseAliasItem,
    TraitUseConflictResolution,
    TraitUse,
    RequireClause,
    ConstDeclaration,
    ConstantDeclarator,
    TypeConstDeclaration,
    DecoratedExpression,
    ParameterDeclaration,
    VariadicParameter,
    OldAttributeSpecification,
    AttributeSpecification,
    Attribute,
    InclusionExpression,
    InclusionDirective,
    CompoundStatement,
    ExpressionStatement,
    MarkupSection,
    MarkupSuffix,
    UnsetStatement,
    LetStatement,
    UsingStatementBlockScoped,
    UsingStatementFunctionScoped,
    WhileStatement,
    IfStatement,
    ElseifClause,
    ElseClause,
    TryStatement,
    CatchClause,
    FinallyClause,
    DoStatement,
    ForStatement,
    ForeachStatement,
    SwitchStatement,
    SwitchSection,
    SwitchFallthrough,
    CaseLabel,
    DefaultLabel,
    ReturnStatement,
    GotoLabel,
    GotoStatement,
    ThrowStatement,
    BreakStatement,
    ContinueStatement,
    EchoStatement,
    ConcurrentStatement,
    SimpleInitializer,
    AnonymousClass,
    AnonymousFunction,
    AnonymousFunctionUseClause,
    LambdaExpression,
    LambdaSignature,
    CastExpression,
    ScopeResolutionExpression,
    MemberSelectionExpression,
    SafeMemberSelectionExpression,
    EmbeddedMemberSelectionExpression,
    YieldExpression,
    YieldFromExpression,
    PrefixUnaryExpression,
    PostfixUnaryExpression,
    BinaryExpression,
    IsExpression,
    AsExpression,
    NullableAsExpression,
    ConditionalExpression,
    EvalExpression,
    DefineExpression,
    HaltCompilerExpression,
    IssetExpression,
    FunctionCallExpression,
    ParenthesizedExpression,
    BracedExpression,
    EmbeddedBracedExpression,
    ListExpression,
    CollectionLiteralExpression,
    ObjectCreationExpression,
    ConstructorCall,
    RecordCreationExpression,
    ArrayCreationExpression,
    ArrayIntrinsicExpression,
    DarrayIntrinsicExpression,
    DictionaryIntrinsicExpression,
    KeysetIntrinsicExpression,
    VarrayIntrinsicExpression,
    VectorIntrinsicExpression,
    ElementInitializer,
    SubscriptExpression,
    EmbeddedSubscriptExpression,
    AwaitableCreationExpression,
    XHPChildrenDeclaration,
    XHPChildrenParenthesizedList,
    XHPCategoryDeclaration,
    XHPEnumType,
    XHPLateinit,
    XHPRequired,
    XHPClassAttributeDeclaration,
    XHPClassAttribute,
    XHPSimpleClassAttribute,
    XHPSimpleAttribute,
    XHPSpreadAttribute,
    XHPOpen,
    XHPExpression,
    XHPClose,
    TypeConstant,
    VectorTypeSpecifier,
    KeysetTypeSpecifier,
    TupleTypeExplicitSpecifier,
    VarrayTypeSpecifier,
    VectorArrayTypeSpecifier,
    TypeParameter,
    TypeConstraint,
    DarrayTypeSpecifier,
    MapArrayTypeSpecifier,
    DictionaryTypeSpecifier,
    ClosureTypeSpecifier,
    ClosureParameterTypeSpecifier,
    ClassnameTypeSpecifier,
    FieldSpecifier,
    FieldInitializer,
    ShapeTypeSpecifier,
    ShapeExpression,
    TupleExpression,
    GenericTypeSpecifier,
    NullableTypeSpecifier,
    LikeTypeSpecifier,
    SoftTypeSpecifier,
    AttributizedSpecifier,
    ReifiedTypeArgument,
    TypeArguments,
    TypeParameters,
    TupleTypeSpecifier,
    ErrorSyntax,
    ListItem,
    PocketAtomExpression,
    PocketIdentifierExpression,
    PocketAtomMappingDeclaration,
    PocketEnumDeclaration,
    PocketFieldTypeExprDeclaration,
    PocketFieldTypeDeclaration,
    PocketMappingIdDeclaration,
    PocketMappingTypeDeclaration,

}

impl SyntaxKind {
    pub fn to_string(&self) -> &str {
        match self {
            SyntaxKind::SyntaxList => "list",
            SyntaxKind::Missing => "missing",
            SyntaxKind::Token(_) => "token",
            SyntaxKind::EndOfFile                         => "end_of_file",
            SyntaxKind::Script                            => "script",
            SyntaxKind::QualifiedName                     => "qualified_name",
            SyntaxKind::SimpleTypeSpecifier               => "simple_type_specifier",
            SyntaxKind::LiteralExpression                 => "literal",
            SyntaxKind::PrefixedStringExpression          => "prefixed_string",
            SyntaxKind::VariableExpression                => "variable",
            SyntaxKind::PipeVariableExpression            => "pipe_variable",
            SyntaxKind::FileAttributeSpecification        => "file_attribute_specification",
            SyntaxKind::EnumDeclaration                   => "enum_declaration",
            SyntaxKind::Enumerator                        => "enumerator",
            SyntaxKind::RecordDeclaration                 => "record_declaration",
            SyntaxKind::RecordField                       => "record_field",
            SyntaxKind::AliasDeclaration                  => "alias_declaration",
            SyntaxKind::PropertyDeclaration               => "property_declaration",
            SyntaxKind::PropertyDeclarator                => "property_declarator",
            SyntaxKind::NamespaceDeclaration              => "namespace_declaration",
            SyntaxKind::NamespaceBody                     => "namespace_body",
            SyntaxKind::NamespaceEmptyBody                => "namespace_empty_body",
            SyntaxKind::NamespaceUseDeclaration           => "namespace_use_declaration",
            SyntaxKind::NamespaceGroupUseDeclaration      => "namespace_group_use_declaration",
            SyntaxKind::NamespaceUseClause                => "namespace_use_clause",
            SyntaxKind::FunctionDeclaration               => "function_declaration",
            SyntaxKind::FunctionDeclarationHeader         => "function_declaration_header",
            SyntaxKind::WhereClause                       => "where_clause",
            SyntaxKind::WhereConstraint                   => "where_constraint",
            SyntaxKind::MethodishDeclaration              => "methodish_declaration",
            SyntaxKind::MethodishTraitResolution          => "methodish_trait_resolution",
            SyntaxKind::ClassishDeclaration               => "classish_declaration",
            SyntaxKind::ClassishBody                      => "classish_body",
            SyntaxKind::TraitUsePrecedenceItem            => "trait_use_precedence_item",
            SyntaxKind::TraitUseAliasItem                 => "trait_use_alias_item",
            SyntaxKind::TraitUseConflictResolution        => "trait_use_conflict_resolution",
            SyntaxKind::TraitUse                          => "trait_use",
            SyntaxKind::RequireClause                     => "require_clause",
            SyntaxKind::ConstDeclaration                  => "const_declaration",
            SyntaxKind::ConstantDeclarator                => "constant_declarator",
            SyntaxKind::TypeConstDeclaration              => "type_const_declaration",
            SyntaxKind::DecoratedExpression               => "decorated_expression",
            SyntaxKind::ParameterDeclaration              => "parameter_declaration",
            SyntaxKind::VariadicParameter                 => "variadic_parameter",
            SyntaxKind::OldAttributeSpecification         => "old_attribute_specification",
            SyntaxKind::AttributeSpecification            => "attribute_specification",
            SyntaxKind::Attribute                         => "attribute",
            SyntaxKind::InclusionExpression               => "inclusion_expression",
            SyntaxKind::InclusionDirective                => "inclusion_directive",
            SyntaxKind::CompoundStatement                 => "compound_statement",
            SyntaxKind::ExpressionStatement               => "expression_statement",
            SyntaxKind::MarkupSection                     => "markup_section",
            SyntaxKind::MarkupSuffix                      => "markup_suffix",
            SyntaxKind::UnsetStatement                    => "unset_statement",
            SyntaxKind::LetStatement                      => "let_statement",
            SyntaxKind::UsingStatementBlockScoped         => "using_statement_block_scoped",
            SyntaxKind::UsingStatementFunctionScoped      => "using_statement_function_scoped",
            SyntaxKind::WhileStatement                    => "while_statement",
            SyntaxKind::IfStatement                       => "if_statement",
            SyntaxKind::ElseifClause                      => "elseif_clause",
            SyntaxKind::ElseClause                        => "else_clause",
            SyntaxKind::TryStatement                      => "try_statement",
            SyntaxKind::CatchClause                       => "catch_clause",
            SyntaxKind::FinallyClause                     => "finally_clause",
            SyntaxKind::DoStatement                       => "do_statement",
            SyntaxKind::ForStatement                      => "for_statement",
            SyntaxKind::ForeachStatement                  => "foreach_statement",
            SyntaxKind::SwitchStatement                   => "switch_statement",
            SyntaxKind::SwitchSection                     => "switch_section",
            SyntaxKind::SwitchFallthrough                 => "switch_fallthrough",
            SyntaxKind::CaseLabel                         => "case_label",
            SyntaxKind::DefaultLabel                      => "default_label",
            SyntaxKind::ReturnStatement                   => "return_statement",
            SyntaxKind::GotoLabel                         => "goto_label",
            SyntaxKind::GotoStatement                     => "goto_statement",
            SyntaxKind::ThrowStatement                    => "throw_statement",
            SyntaxKind::BreakStatement                    => "break_statement",
            SyntaxKind::ContinueStatement                 => "continue_statement",
            SyntaxKind::EchoStatement                     => "echo_statement",
            SyntaxKind::ConcurrentStatement               => "concurrent_statement",
            SyntaxKind::SimpleInitializer                 => "simple_initializer",
            SyntaxKind::AnonymousClass                    => "anonymous_class",
            SyntaxKind::AnonymousFunction                 => "anonymous_function",
            SyntaxKind::AnonymousFunctionUseClause        => "anonymous_function_use_clause",
            SyntaxKind::LambdaExpression                  => "lambda_expression",
            SyntaxKind::LambdaSignature                   => "lambda_signature",
            SyntaxKind::CastExpression                    => "cast_expression",
            SyntaxKind::ScopeResolutionExpression         => "scope_resolution_expression",
            SyntaxKind::MemberSelectionExpression         => "member_selection_expression",
            SyntaxKind::SafeMemberSelectionExpression     => "safe_member_selection_expression",
            SyntaxKind::EmbeddedMemberSelectionExpression => "embedded_member_selection_expression",
            SyntaxKind::YieldExpression                   => "yield_expression",
            SyntaxKind::YieldFromExpression               => "yield_from_expression",
            SyntaxKind::PrefixUnaryExpression             => "prefix_unary_expression",
            SyntaxKind::PostfixUnaryExpression            => "postfix_unary_expression",
            SyntaxKind::BinaryExpression                  => "binary_expression",
            SyntaxKind::IsExpression                      => "is_expression",
            SyntaxKind::AsExpression                      => "as_expression",
            SyntaxKind::NullableAsExpression              => "nullable_as_expression",
            SyntaxKind::ConditionalExpression             => "conditional_expression",
            SyntaxKind::EvalExpression                    => "eval_expression",
            SyntaxKind::DefineExpression                  => "define_expression",
            SyntaxKind::HaltCompilerExpression            => "halt_compiler_expression",
            SyntaxKind::IssetExpression                   => "isset_expression",
            SyntaxKind::FunctionCallExpression            => "function_call_expression",
            SyntaxKind::ParenthesizedExpression           => "parenthesized_expression",
            SyntaxKind::BracedExpression                  => "braced_expression",
            SyntaxKind::EmbeddedBracedExpression          => "embedded_braced_expression",
            SyntaxKind::ListExpression                    => "list_expression",
            SyntaxKind::CollectionLiteralExpression       => "collection_literal_expression",
            SyntaxKind::ObjectCreationExpression          => "object_creation_expression",
            SyntaxKind::ConstructorCall                   => "constructor_call",
            SyntaxKind::RecordCreationExpression          => "record_creation_expression",
            SyntaxKind::ArrayCreationExpression           => "array_creation_expression",
            SyntaxKind::ArrayIntrinsicExpression          => "array_intrinsic_expression",
            SyntaxKind::DarrayIntrinsicExpression         => "darray_intrinsic_expression",
            SyntaxKind::DictionaryIntrinsicExpression     => "dictionary_intrinsic_expression",
            SyntaxKind::KeysetIntrinsicExpression         => "keyset_intrinsic_expression",
            SyntaxKind::VarrayIntrinsicExpression         => "varray_intrinsic_expression",
            SyntaxKind::VectorIntrinsicExpression         => "vector_intrinsic_expression",
            SyntaxKind::ElementInitializer                => "element_initializer",
            SyntaxKind::SubscriptExpression               => "subscript_expression",
            SyntaxKind::EmbeddedSubscriptExpression       => "embedded_subscript_expression",
            SyntaxKind::AwaitableCreationExpression       => "awaitable_creation_expression",
            SyntaxKind::XHPChildrenDeclaration            => "xhp_children_declaration",
            SyntaxKind::XHPChildrenParenthesizedList      => "xhp_children_parenthesized_list",
            SyntaxKind::XHPCategoryDeclaration            => "xhp_category_declaration",
            SyntaxKind::XHPEnumType                       => "xhp_enum_type",
            SyntaxKind::XHPLateinit                       => "xhp_lateinit",
            SyntaxKind::XHPRequired                       => "xhp_required",
            SyntaxKind::XHPClassAttributeDeclaration      => "xhp_class_attribute_declaration",
            SyntaxKind::XHPClassAttribute                 => "xhp_class_attribute",
            SyntaxKind::XHPSimpleClassAttribute           => "xhp_simple_class_attribute",
            SyntaxKind::XHPSimpleAttribute                => "xhp_simple_attribute",
            SyntaxKind::XHPSpreadAttribute                => "xhp_spread_attribute",
            SyntaxKind::XHPOpen                           => "xhp_open",
            SyntaxKind::XHPExpression                     => "xhp_expression",
            SyntaxKind::XHPClose                          => "xhp_close",
            SyntaxKind::TypeConstant                      => "type_constant",
            SyntaxKind::VectorTypeSpecifier               => "vector_type_specifier",
            SyntaxKind::KeysetTypeSpecifier               => "keyset_type_specifier",
            SyntaxKind::TupleTypeExplicitSpecifier        => "tuple_type_explicit_specifier",
            SyntaxKind::VarrayTypeSpecifier               => "varray_type_specifier",
            SyntaxKind::VectorArrayTypeSpecifier          => "vector_array_type_specifier",
            SyntaxKind::TypeParameter                     => "type_parameter",
            SyntaxKind::TypeConstraint                    => "type_constraint",
            SyntaxKind::DarrayTypeSpecifier               => "darray_type_specifier",
            SyntaxKind::MapArrayTypeSpecifier             => "map_array_type_specifier",
            SyntaxKind::DictionaryTypeSpecifier           => "dictionary_type_specifier",
            SyntaxKind::ClosureTypeSpecifier              => "closure_type_specifier",
            SyntaxKind::ClosureParameterTypeSpecifier     => "closure_parameter_type_specifier",
            SyntaxKind::ClassnameTypeSpecifier            => "classname_type_specifier",
            SyntaxKind::FieldSpecifier                    => "field_specifier",
            SyntaxKind::FieldInitializer                  => "field_initializer",
            SyntaxKind::ShapeTypeSpecifier                => "shape_type_specifier",
            SyntaxKind::ShapeExpression                   => "shape_expression",
            SyntaxKind::TupleExpression                   => "tuple_expression",
            SyntaxKind::GenericTypeSpecifier              => "generic_type_specifier",
            SyntaxKind::NullableTypeSpecifier             => "nullable_type_specifier",
            SyntaxKind::LikeTypeSpecifier                 => "like_type_specifier",
            SyntaxKind::SoftTypeSpecifier                 => "soft_type_specifier",
            SyntaxKind::AttributizedSpecifier             => "attributized_specifier",
            SyntaxKind::ReifiedTypeArgument               => "reified_type_argument",
            SyntaxKind::TypeArguments                     => "type_arguments",
            SyntaxKind::TypeParameters                    => "type_parameters",
            SyntaxKind::TupleTypeSpecifier                => "tuple_type_specifier",
            SyntaxKind::ErrorSyntax                       => "error",
            SyntaxKind::ListItem                          => "list_item",
            SyntaxKind::PocketAtomExpression              => "pocket_atom",
            SyntaxKind::PocketIdentifierExpression        => "pocket_identifier",
            SyntaxKind::PocketAtomMappingDeclaration      => "pocket_atom_mapping",
            SyntaxKind::PocketEnumDeclaration             => "pocket_enum_declaration",
            SyntaxKind::PocketFieldTypeExprDeclaration    => "pocket_field_type_expr_declaration",
            SyntaxKind::PocketFieldTypeDeclaration        => "pocket_field_type_declaration",
            SyntaxKind::PocketMappingIdDeclaration        => "pocket_mapping_id_declaration",
            SyntaxKind::PocketMappingTypeDeclaration      => "pocket_mapping_type_declaration",
        }
    }

    pub fn ocaml_tag(self) -> u8 {
        match self {
            SyntaxKind::Missing => 0,
            SyntaxKind::Token(_) => 0,
            SyntaxKind::SyntaxList => 1,
            SyntaxKind::EndOfFile => 2,
            SyntaxKind::Script => 3,
            SyntaxKind::QualifiedName => 4,
            SyntaxKind::SimpleTypeSpecifier => 5,
            SyntaxKind::LiteralExpression => 6,
            SyntaxKind::PrefixedStringExpression => 7,
            SyntaxKind::VariableExpression => 8,
            SyntaxKind::PipeVariableExpression => 9,
            SyntaxKind::FileAttributeSpecification => 10,
            SyntaxKind::EnumDeclaration => 11,
            SyntaxKind::Enumerator => 12,
            SyntaxKind::RecordDeclaration => 13,
            SyntaxKind::RecordField => 14,
            SyntaxKind::AliasDeclaration => 15,
            SyntaxKind::PropertyDeclaration => 16,
            SyntaxKind::PropertyDeclarator => 17,
            SyntaxKind::NamespaceDeclaration => 18,
            SyntaxKind::NamespaceBody => 19,
            SyntaxKind::NamespaceEmptyBody => 20,
            SyntaxKind::NamespaceUseDeclaration => 21,
            SyntaxKind::NamespaceGroupUseDeclaration => 22,
            SyntaxKind::NamespaceUseClause => 23,
            SyntaxKind::FunctionDeclaration => 24,
            SyntaxKind::FunctionDeclarationHeader => 25,
            SyntaxKind::WhereClause => 26,
            SyntaxKind::WhereConstraint => 27,
            SyntaxKind::MethodishDeclaration => 28,
            SyntaxKind::MethodishTraitResolution => 29,
            SyntaxKind::ClassishDeclaration => 30,
            SyntaxKind::ClassishBody => 31,
            SyntaxKind::TraitUsePrecedenceItem => 32,
            SyntaxKind::TraitUseAliasItem => 33,
            SyntaxKind::TraitUseConflictResolution => 34,
            SyntaxKind::TraitUse => 35,
            SyntaxKind::RequireClause => 36,
            SyntaxKind::ConstDeclaration => 37,
            SyntaxKind::ConstantDeclarator => 38,
            SyntaxKind::TypeConstDeclaration => 39,
            SyntaxKind::DecoratedExpression => 40,
            SyntaxKind::ParameterDeclaration => 41,
            SyntaxKind::VariadicParameter => 42,
            SyntaxKind::OldAttributeSpecification => 43,
            SyntaxKind::AttributeSpecification => 44,
            SyntaxKind::Attribute => 45,
            SyntaxKind::InclusionExpression => 46,
            SyntaxKind::InclusionDirective => 47,
            SyntaxKind::CompoundStatement => 48,
            SyntaxKind::ExpressionStatement => 49,
            SyntaxKind::MarkupSection => 50,
            SyntaxKind::MarkupSuffix => 51,
            SyntaxKind::UnsetStatement => 52,
            SyntaxKind::LetStatement => 53,
            SyntaxKind::UsingStatementBlockScoped => 54,
            SyntaxKind::UsingStatementFunctionScoped => 55,
            SyntaxKind::WhileStatement => 56,
            SyntaxKind::IfStatement => 57,
            SyntaxKind::ElseifClause => 58,
            SyntaxKind::ElseClause => 59,
            SyntaxKind::TryStatement => 60,
            SyntaxKind::CatchClause => 61,
            SyntaxKind::FinallyClause => 62,
            SyntaxKind::DoStatement => 63,
            SyntaxKind::ForStatement => 64,
            SyntaxKind::ForeachStatement => 65,
            SyntaxKind::SwitchStatement => 66,
            SyntaxKind::SwitchSection => 67,
            SyntaxKind::SwitchFallthrough => 68,
            SyntaxKind::CaseLabel => 69,
            SyntaxKind::DefaultLabel => 70,
            SyntaxKind::ReturnStatement => 71,
            SyntaxKind::GotoLabel => 72,
            SyntaxKind::GotoStatement => 73,
            SyntaxKind::ThrowStatement => 74,
            SyntaxKind::BreakStatement => 75,
            SyntaxKind::ContinueStatement => 76,
            SyntaxKind::EchoStatement => 77,
            SyntaxKind::ConcurrentStatement => 78,
            SyntaxKind::SimpleInitializer => 79,
            SyntaxKind::AnonymousClass => 80,
            SyntaxKind::AnonymousFunction => 81,
            SyntaxKind::AnonymousFunctionUseClause => 82,
            SyntaxKind::LambdaExpression => 83,
            SyntaxKind::LambdaSignature => 84,
            SyntaxKind::CastExpression => 85,
            SyntaxKind::ScopeResolutionExpression => 86,
            SyntaxKind::MemberSelectionExpression => 87,
            SyntaxKind::SafeMemberSelectionExpression => 88,
            SyntaxKind::EmbeddedMemberSelectionExpression => 89,
            SyntaxKind::YieldExpression => 90,
            SyntaxKind::YieldFromExpression => 91,
            SyntaxKind::PrefixUnaryExpression => 92,
            SyntaxKind::PostfixUnaryExpression => 93,
            SyntaxKind::BinaryExpression => 94,
            SyntaxKind::IsExpression => 95,
            SyntaxKind::AsExpression => 96,
            SyntaxKind::NullableAsExpression => 97,
            SyntaxKind::ConditionalExpression => 98,
            SyntaxKind::EvalExpression => 99,
            SyntaxKind::DefineExpression => 100,
            SyntaxKind::HaltCompilerExpression => 101,
            SyntaxKind::IssetExpression => 102,
            SyntaxKind::FunctionCallExpression => 103,
            SyntaxKind::ParenthesizedExpression => 104,
            SyntaxKind::BracedExpression => 105,
            SyntaxKind::EmbeddedBracedExpression => 106,
            SyntaxKind::ListExpression => 107,
            SyntaxKind::CollectionLiteralExpression => 108,
            SyntaxKind::ObjectCreationExpression => 109,
            SyntaxKind::ConstructorCall => 110,
            SyntaxKind::RecordCreationExpression => 111,
            SyntaxKind::ArrayCreationExpression => 112,
            SyntaxKind::ArrayIntrinsicExpression => 113,
            SyntaxKind::DarrayIntrinsicExpression => 114,
            SyntaxKind::DictionaryIntrinsicExpression => 115,
            SyntaxKind::KeysetIntrinsicExpression => 116,
            SyntaxKind::VarrayIntrinsicExpression => 117,
            SyntaxKind::VectorIntrinsicExpression => 118,
            SyntaxKind::ElementInitializer => 119,
            SyntaxKind::SubscriptExpression => 120,
            SyntaxKind::EmbeddedSubscriptExpression => 121,
            SyntaxKind::AwaitableCreationExpression => 122,
            SyntaxKind::XHPChildrenDeclaration => 123,
            SyntaxKind::XHPChildrenParenthesizedList => 124,
            SyntaxKind::XHPCategoryDeclaration => 125,
            SyntaxKind::XHPEnumType => 126,
            SyntaxKind::XHPLateinit => 127,
            SyntaxKind::XHPRequired => 128,
            SyntaxKind::XHPClassAttributeDeclaration => 129,
            SyntaxKind::XHPClassAttribute => 130,
            SyntaxKind::XHPSimpleClassAttribute => 131,
            SyntaxKind::XHPSimpleAttribute => 132,
            SyntaxKind::XHPSpreadAttribute => 133,
            SyntaxKind::XHPOpen => 134,
            SyntaxKind::XHPExpression => 135,
            SyntaxKind::XHPClose => 136,
            SyntaxKind::TypeConstant => 137,
            SyntaxKind::VectorTypeSpecifier => 138,
            SyntaxKind::KeysetTypeSpecifier => 139,
            SyntaxKind::TupleTypeExplicitSpecifier => 140,
            SyntaxKind::VarrayTypeSpecifier => 141,
            SyntaxKind::VectorArrayTypeSpecifier => 142,
            SyntaxKind::TypeParameter => 143,
            SyntaxKind::TypeConstraint => 144,
            SyntaxKind::DarrayTypeSpecifier => 145,
            SyntaxKind::MapArrayTypeSpecifier => 146,
            SyntaxKind::DictionaryTypeSpecifier => 147,
            SyntaxKind::ClosureTypeSpecifier => 148,
            SyntaxKind::ClosureParameterTypeSpecifier => 149,
            SyntaxKind::ClassnameTypeSpecifier => 150,
            SyntaxKind::FieldSpecifier => 151,
            SyntaxKind::FieldInitializer => 152,
            SyntaxKind::ShapeTypeSpecifier => 153,
            SyntaxKind::ShapeExpression => 154,
            SyntaxKind::TupleExpression => 155,
            SyntaxKind::GenericTypeSpecifier => 156,
            SyntaxKind::NullableTypeSpecifier => 157,
            SyntaxKind::LikeTypeSpecifier => 158,
            SyntaxKind::SoftTypeSpecifier => 159,
            SyntaxKind::AttributizedSpecifier => 160,
            SyntaxKind::ReifiedTypeArgument => 161,
            SyntaxKind::TypeArguments => 162,
            SyntaxKind::TypeParameters => 163,
            SyntaxKind::TupleTypeSpecifier => 164,
            SyntaxKind::ErrorSyntax => 165,
            SyntaxKind::ListItem => 166,
            SyntaxKind::PocketAtomExpression => 167,
            SyntaxKind::PocketIdentifierExpression => 168,
            SyntaxKind::PocketAtomMappingDeclaration => 169,
            SyntaxKind::PocketEnumDeclaration => 170,
            SyntaxKind::PocketFieldTypeExprDeclaration => 171,
            SyntaxKind::PocketFieldTypeDeclaration => 172,
            SyntaxKind::PocketMappingIdDeclaration => 173,
            SyntaxKind::PocketMappingTypeDeclaration => 174,
        }
    }
}
