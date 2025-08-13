use clang::{Entity, EntityKind, Type, TypeKind};

#[derive(Debug, Clone)]
pub struct CppAst {
    pub functions: Vec<Function>,
    pub global_variables: Vec<Variable>,
}

impl CppAst {
    pub fn new() -> Self {
        Self {
            functions: Vec::new(),
            global_variables: Vec::new(),
        }
    }
}

#[derive(Debug, Clone)]
pub struct Function {
    pub name: String,
    pub parameters: Vec<Variable>,
    #[allow(dead_code)]
    pub return_type: String,
    #[allow(dead_code)]
    pub body: Vec<Statement>,
    #[allow(dead_code)]
    pub location: SourceLocation,
}

#[derive(Debug, Clone)]
pub struct Variable {
    pub name: String,
    pub type_name: String,
    pub is_reference: bool,
    #[allow(dead_code)]
    pub is_pointer: bool,
    pub is_const: bool,
    pub is_unique_ptr: bool,
    #[allow(dead_code)]
    pub is_shared_ptr: bool,
    #[allow(dead_code)]
    pub location: SourceLocation,
}

#[derive(Debug, Clone)]
#[allow(dead_code)]
pub enum Statement {
    VariableDecl(Variable),
    Assignment {
        lhs: String,
        rhs: Expression,
        location: SourceLocation,
    },
    ReferenceBinding {
        name: String,
        target: Expression,
        is_mutable: bool,
        location: SourceLocation,
    },
    Return(Option<Expression>),
    FunctionCall {
        name: String,
        args: Vec<Expression>,
        location: SourceLocation,
    },
    Block(Vec<Statement>),
}

#[derive(Debug, Clone)]
#[allow(dead_code)]
pub enum Expression {
    Variable(String),
    Move(Box<Expression>),
    Dereference(Box<Expression>),
    AddressOf(Box<Expression>),
    FunctionCall {
        name: String,
        args: Vec<Expression>,
    },
    Literal(String),
}

#[derive(Debug, Clone)]
pub struct SourceLocation {
    #[allow(dead_code)]
    pub file: String,
    #[allow(dead_code)]
    pub line: u32,
    #[allow(dead_code)]
    pub column: u32,
}

pub fn extract_function(entity: &Entity) -> Function {
    let name = entity.get_name().unwrap_or_else(|| "anonymous".to_string());
    let location = extract_location(entity);
    
    let mut parameters = Vec::new();
    for child in entity.get_children() {
        if child.get_kind() == EntityKind::ParmDecl {
            parameters.push(extract_variable(&child));
        }
    }
    
    let return_type = entity
        .get_result_type()
        .map(|t| type_to_string(&t))
        .unwrap_or_else(|| "void".to_string());
    
    let body = extract_function_body(entity);
    
    Function {
        name,
        parameters,
        return_type,
        body,
        location,
    }
}

pub fn extract_variable(entity: &Entity) -> Variable {
    let name = entity.get_name().unwrap_or_else(|| "anonymous".to_string());
    let location = extract_location(entity);
    
    let type_info = entity.get_type().unwrap();
    let type_name = type_to_string(&type_info);
    
    let is_reference = matches!(type_info.get_kind(), TypeKind::LValueReference | TypeKind::RValueReference);
    let is_pointer = matches!(type_info.get_kind(), TypeKind::Pointer);
    
    // For references, check if the pointee type is const
    let is_const = if is_reference {
        if let Some(pointee) = type_info.get_pointee_type() {
            pointee.is_const_qualified()
        } else {
            type_info.is_const_qualified()
        }
    } else {
        type_info.is_const_qualified()
    };
    
    let is_unique_ptr = type_name.contains("unique_ptr");
    let is_shared_ptr = type_name.contains("shared_ptr");
    
    Variable {
        name,
        type_name,
        is_reference,
        is_pointer,
        is_const,
        is_unique_ptr,
        is_shared_ptr,
        location,
    }
}

fn extract_function_body(entity: &Entity) -> Vec<Statement> {
    let mut statements = Vec::new();
    
    for child in entity.get_children() {
        if child.get_kind() == EntityKind::CompoundStmt {
            statements.extend(extract_compound_statement(&child));
        }
    }
    
    statements
}

fn extract_compound_statement(entity: &Entity) -> Vec<Statement> {
    let mut statements = Vec::new();
    
    for child in entity.get_children() {
        match child.get_kind() {
            EntityKind::DeclStmt => {
                for decl_child in child.get_children() {
                    if decl_child.get_kind() == EntityKind::VarDecl {
                        let var = extract_variable(&decl_child);
                        
                        // Always add the variable declaration first
                        statements.push(Statement::VariableDecl(var.clone()));
                        
                        // Check if this is a reference binding with initialization
                        if var.is_reference {
                            // Try to find what it references
                            for child in decl_child.get_children() {
                                if let Some(expr) = extract_expression(&child) {
                                    statements.push(Statement::ReferenceBinding {
                                        name: var.name.clone(),
                                        target: expr,
                                        is_mutable: !var.is_const,
                                        location: extract_location(&decl_child),
                                    });
                                    break;
                                }
                            }
                        }
                    }
                }
            }
            EntityKind::BinaryOperator => {
                // Handle assignments
                let children: Vec<Entity> = child.get_children().into_iter().collect();
                if children.len() == 2 {
                    if let (Some(lhs_expr), Some(rhs_expr)) = 
                        (extract_expression(&children[0]), extract_expression(&children[1])) {
                        statements.push(Statement::Assignment {
                            lhs: format!("{:?}", lhs_expr), // Simplified for now
                            rhs: rhs_expr,
                            location: extract_location(&child),
                        });
                    }
                }
            }
            EntityKind::CallExpr => {
                let children: Vec<Entity> = child.get_children().into_iter().collect();
                let mut name = "unknown".to_string();
                let mut args = Vec::new();
                
                for c in children {
                    match c.get_kind() {
                        EntityKind::DeclRefExpr | EntityKind::UnexposedExpr => {
                            if name == "unknown" {
                                if let Some(n) = c.get_name() {
                                    name = n;
                                }
                            } else {
                                // This is an argument
                                if let Some(expr) = extract_expression(&c) {
                                    args.push(expr);
                                }
                            }
                        }
                        _ => {
                            // Try to extract as argument
                            if let Some(expr) = extract_expression(&c) {
                                args.push(expr);
                            }
                        }
                    }
                }
                
                statements.push(Statement::FunctionCall {
                    name,
                    args,
                    location: extract_location(&child),
                });
            }
            EntityKind::ReturnStmt => {
                statements.push(Statement::Return(None));
            }
            _ => {}
        }
    }
    
    statements
}

fn extract_expression(entity: &Entity) -> Option<Expression> {
    match entity.get_kind() {
        EntityKind::DeclRefExpr => {
            entity.get_name().map(Expression::Variable)
        }
        EntityKind::CallExpr => {
            // Extract function call as expression
            let children: Vec<Entity> = entity.get_children().into_iter().collect();
            let mut name = "unknown".to_string();
            let mut args = Vec::new();
            
            for c in children {
                match c.get_kind() {
                    EntityKind::DeclRefExpr | EntityKind::UnexposedExpr => {
                        if name == "unknown" {
                            if let Some(n) = c.get_name() {
                                name = n;
                            }
                        } else {
                            if let Some(expr) = extract_expression(&c) {
                                args.push(expr);
                            }
                        }
                    }
                    _ => {
                        if let Some(expr) = extract_expression(&c) {
                            args.push(expr);
                        }
                    }
                }
            }
            
            Some(Expression::FunctionCall { name, args })
        }
        EntityKind::UnexposedExpr => {
            // UnexposedExpr often wraps other expressions, so look at its children
            for child in entity.get_children() {
                if let Some(expr) = extract_expression(&child) {
                    return Some(expr);
                }
            }
            None
        }
        EntityKind::UnaryOperator => {
            // Check if it's address-of (&) or dereference (*)
            let children: Vec<Entity> = entity.get_children().into_iter().collect();
            if !children.is_empty() {
                if let Some(inner) = extract_expression(&children[0]) {
                    // Simplified - would need to check operator type
                    return Some(Expression::AddressOf(Box::new(inner)));
                }
            }
            None
        }
        _ => None
    }
}

fn extract_location(entity: &Entity) -> SourceLocation {
    let location = entity.get_location().unwrap();
    let file_location = location.get_file_location();
    
    SourceLocation {
        file: file_location
            .file
            .map(|f| f.get_path().display().to_string())
            .unwrap_or_else(|| "unknown".to_string()),
        line: file_location.line,
        column: file_location.column,
    }
}

fn type_to_string(ty: &Type) -> String {
    ty.get_display_name()
}