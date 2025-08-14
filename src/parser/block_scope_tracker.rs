// A simpler fix: Track when we enter/exit {} blocks in the parser
// This could be added to the existing parser with minimal changes

use clang::{Entity, EntityKind};

#[derive(Debug, Clone)]
pub enum ScopedStatement {
    // Regular statement
    Statement(super::Statement),
    // Block with its own scope
    Block {
        statements: Vec<ScopedStatement>,
    },
    // If with scoped branches
    If {
        condition: super::Expression,
        then_branch: Box<ScopedStatement>,
        else_branch: Option<Box<ScopedStatement>>,
    },
    // Loop with scoped body
    Loop {
        kind: LoopKind,
        condition: Option<super::Expression>,
        body: Box<ScopedStatement>,
    },
}

#[derive(Debug, Clone)]
pub enum LoopKind {
    For,
    While,
    DoWhile,
}

/// Enhanced statement extraction that preserves scope information
pub fn extract_scoped_statement(entity: &Entity) -> Option<ScopedStatement> {
    match entity.get_kind() {
        EntityKind::CompoundStmt => {
            // This is a {} block - create a scope
            let mut statements = Vec::new();
            for child in entity.get_children() {
                if let Some(stmt) = extract_scoped_statement(&child) {
                    statements.push(stmt);
                }
            }
            Some(ScopedStatement::Block { statements })
        }
        
        EntityKind::IfStmt => {
            let children: Vec<Entity> = entity.get_children().collect();
            if children.len() >= 2 {
                // First child is condition, second is then-branch
                let condition = extract_expression(&children[0])
                    .unwrap_or(super::Expression::Literal("true".to_string()));
                
                let then_branch = Box::new(
                    extract_scoped_statement(&children[1])
                        .unwrap_or(ScopedStatement::Block { statements: vec![] })
                );
                
                let else_branch = if children.len() > 2 {
                    Some(Box::new(
                        extract_scoped_statement(&children[2])
                            .unwrap_or(ScopedStatement::Block { statements: vec![] })
                    ))
                } else {
                    None
                };
                
                Some(ScopedStatement::If {
                    condition,
                    then_branch,
                    else_branch,
                })
            } else {
                None
            }
        }
        
        EntityKind::ForStmt => {
            // For loop: init, condition, increment, body
            let children: Vec<Entity> = entity.get_children().collect();
            
            // Find the body (usually last child that's a CompoundStmt)
            let body = children.iter()
                .rev()
                .find(|c| c.get_kind() == EntityKind::CompoundStmt)
                .and_then(|c| extract_scoped_statement(c))
                .unwrap_or(ScopedStatement::Block { statements: vec![] });
            
            Some(ScopedStatement::Loop {
                kind: LoopKind::For,
                condition: None, // Could extract if needed
                body: Box::new(body),
            })
        }
        
        EntityKind::WhileStmt => {
            let children: Vec<Entity> = entity.get_children().collect();
            
            let condition = children.get(0)
                .and_then(|c| extract_expression(c));
            
            let body = children.get(1)
                .and_then(|c| extract_scoped_statement(c))
                .unwrap_or(ScopedStatement::Block { statements: vec![] });
            
            Some(ScopedStatement::Loop {
                kind: LoopKind::While,
                condition,
                body: Box::new(body),
            })
        }
        
        // For regular statements, wrap in Statement variant
        _ => {
            extract_regular_statement(entity)
                .map(ScopedStatement::Statement)
        }
    }
}

fn extract_expression(entity: &Entity) -> Option<super::Expression> {
    // Simplified - would reuse existing expression extraction
    match entity.get_kind() {
        EntityKind::DeclRefExpr => {
            entity.get_name().map(super::Expression::Variable)
        }
        _ => Some(super::Expression::Literal("unknown".to_string()))
    }
}

fn extract_regular_statement(entity: &Entity) -> Option<super::Statement> {
    // This would call the existing statement extraction logic
    // Just a placeholder for demonstration
    match entity.get_kind() {
        EntityKind::DeclStmt => {
            // Extract variable declaration
            Some(super::Statement::Block(vec![]))
        }
        _ => None
    }
}

/// Convert scoped statements to IR with scope markers
pub fn convert_scoped_to_ir(stmt: &ScopedStatement) -> Vec<IrScopedStatement> {
    match stmt {
        ScopedStatement::Statement(s) => {
            // Regular statement - convert normally
            vec![IrScopedStatement::Statement(convert_statement(s))]
        }
        
        ScopedStatement::Block { statements } => {
            let mut result = vec![IrScopedStatement::EnterScope];
            
            for s in statements {
                result.extend(convert_scoped_to_ir(s));
            }
            
            result.push(IrScopedStatement::ExitScope);
            result
        }
        
        ScopedStatement::If { condition, then_branch, else_branch } => {
            let mut result = vec![];
            
            // Mark beginning of conditional
            result.push(IrScopedStatement::BeginConditional);
            
            // Then branch with scope
            result.push(IrScopedStatement::EnterScope);
            result.extend(convert_scoped_to_ir(then_branch));
            result.push(IrScopedStatement::ExitScope);
            
            // Else branch with scope
            if let Some(else_b) = else_branch {
                result.push(IrScopedStatement::EnterScope);
                result.extend(convert_scoped_to_ir(else_b));
                result.push(IrScopedStatement::ExitScope);
            }
            
            result.push(IrScopedStatement::EndConditional);
            result
        }
        
        ScopedStatement::Loop { kind, condition, body } => {
            let mut result = vec![];
            
            // Mark loop for special handling
            result.push(IrScopedStatement::BeginLoop);
            
            // Loop body is in its own scope each iteration
            result.push(IrScopedStatement::EnterScope);
            result.extend(convert_scoped_to_ir(body));
            result.push(IrScopedStatement::ExitScope);
            
            result.push(IrScopedStatement::EndLoop);
            result
        }
    }
}

#[derive(Debug, Clone)]
pub enum IrScopedStatement {
    Statement(crate::ir::IrStatement),
    EnterScope,
    ExitScope,
    BeginConditional,
    EndConditional,
    BeginLoop,
    EndLoop,
}

fn convert_statement(stmt: &super::Statement) -> crate::ir::IrStatement {
    // Placeholder - would use existing conversion
    crate::ir::IrStatement::Drop("placeholder".to_string())
}

// How to integrate this into the analysis:
// 
// pub fn analyze_with_scopes(statements: Vec<IrScopedStatement>) -> Vec<String> {
//     let mut errors = Vec::new();
//     let mut scope_depth = 0;
//     let mut scope_borrows: Vec<HashSet<String>> = vec![HashSet::new()];
//     
//     for stmt in statements {
//         match stmt {
//             IrScopedStatement::EnterScope => {
//                 scope_depth += 1;
//                 scope_borrows.push(HashSet::new());
//             }
//             IrScopedStatement::ExitScope => {
//                 // Remove all borrows from this scope
//                 if let Some(borrows) = scope_borrows.pop() {
//                     // These borrows are now invalid
//                     for borrow in borrows {
//                         // Clean up borrow tracking
//                     }
//                 }
//                 scope_depth -= 1;
//             }
//             IrScopedStatement::Statement(s) => {
//                 // Process normally but track which scope borrows are in
//             }
//             _ => {}
//         }
//     }
//     
//     errors
// }