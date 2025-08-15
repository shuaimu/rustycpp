use crate::parser::{Statement, Expression, Function};

/// Check for unsafe pointer operations in a function's AST
pub fn check_function_for_pointers(function: &crate::ir::IrFunction) -> Result<Vec<String>, String> {
    // For now, return empty - we need to check at AST level
    // The IR doesn't preserve all the pointer operations
    Ok(Vec::new())
}

/// Check for unsafe pointer operations in a parsed function
pub fn check_parsed_function_for_pointers(function: &Function) -> Vec<String> {
    let mut errors = Vec::new();
    
    for stmt in &function.body {
        if let Some(error) = check_parsed_statement_for_pointers(stmt) {
            errors.push(format!("In function '{}': {}", function.name, error));
        }
    }
    
    errors
}

/// Check if a parsed statement contains pointer operations
pub fn check_parsed_statement_for_pointers(stmt: &Statement) -> Option<String> {
    use crate::parser::Statement;
    
    match stmt {
        Statement::Assignment { rhs, location, .. } => {
            if let Some(op) = contains_pointer_operation(rhs) {
                return Some(format!(
                    "Unsafe pointer {} at line {}: pointer operations require unsafe context",
                    op, location.line
                ));
            }
        }
        Statement::VariableDecl(var) if var.is_pointer => {
            // Raw pointer declaration is allowed, but dereferencing isn't
            return None;
        }
        Statement::FunctionCall { args, location, .. } => {
            for arg in args {
                if let Some(op) = contains_pointer_operation(arg) {
                    return Some(format!(
                        "Unsafe pointer {} in function call at line {}: pointer operations require unsafe context",
                        op, location.line
                    ));
                }
            }
        }
        Statement::Return(Some(expr)) => {
            if let Some(op) = contains_pointer_operation(expr) {
                return Some(format!(
                    "Unsafe pointer {} in return statement: pointer operations require unsafe context",
                    op
                ));
            }
        }
        Statement::If { condition, location, .. } => {
            if let Some(op) = contains_pointer_operation(condition) {
                return Some(format!(
                    "Unsafe pointer {} in condition at line {}: pointer operations require unsafe context", 
                    op, location.line
                ));
            }
        }
        _ => {}
    }
    
    None
}

fn contains_pointer_operation(expr: &Expression) -> Option<&'static str> {
    use crate::parser::Expression;
    
    match expr {
        Expression::Dereference(_) => Some("dereference"),
        Expression::AddressOf(_) => {
            // Taking address is generally safe in Rust, but we can make it require unsafe too
            Some("address-of")
        }
        Expression::FunctionCall { args, .. } => {
            // Check arguments recursively
            for arg in args {
                if let Some(op) = contains_pointer_operation(arg) {
                    return Some(op);
                }
            }
            None
        }
        Expression::BinaryOp { left, right, .. } => {
            // Check both sides
            if let Some(op) = contains_pointer_operation(left) {
                return Some(op);
            }
            contains_pointer_operation(right)
        }
        _ => None
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::parser::{Expression, Statement, SourceLocation, Variable};
    
    #[test]
    fn test_detect_dereference() {
        let expr = Expression::Dereference(Box::new(Expression::Variable("ptr".to_string())));
        assert_eq!(contains_pointer_operation(&expr), Some("dereference"));
    }
    
    #[test]
    fn test_detect_address_of() {
        let expr = Expression::AddressOf(Box::new(Expression::Variable("x".to_string())));
        assert_eq!(contains_pointer_operation(&expr), Some("address-of"));
    }
    
    #[test]
    fn test_safe_expression() {
        let expr = Expression::Variable("x".to_string());
        assert_eq!(contains_pointer_operation(&expr), None);
    }
    
    #[test]
    fn test_pointer_in_assignment() {
        let stmt = Statement::Assignment {
            lhs: "x".to_string(),
            rhs: Expression::Dereference(Box::new(Expression::Variable("ptr".to_string()))),
            location: SourceLocation {
                file: "test.cpp".to_string(),
                line: 10,
                column: 5,
            },
        };
        
        let error = check_parsed_statement_for_pointers(&stmt);
        assert!(error.is_some());
        assert!(error.unwrap().contains("dereference"));
    }
}