use crate::ir::{IrProgram, IrFunction, OwnershipState, BorrowKind};
use crate::parser::HeaderCache;
use std::collections::{HashMap, HashSet};

pub mod ownership;
pub mod borrows;
pub mod lifetimes;
pub mod lifetime_checker;
pub mod scope_lifetime;
pub mod lifetime_inference;

#[derive(Debug, Clone)]
#[allow(dead_code)]
pub struct BorrowCheckError {
    pub kind: ErrorKind,
    pub location: String,
    pub message: String,
}

#[derive(Debug, Clone)]
#[allow(dead_code)]
pub enum ErrorKind {
    UseAfterMove,
    DoubleBorrow,
    MutableBorrowWhileImmutable,
    DanglingReference,
    LifetimeViolation,
}

pub fn check_borrows(program: IrProgram) -> Result<Vec<String>, String> {
    let mut errors = Vec::new();
    
    for function in &program.functions {
        let function_errors = check_function(function)?;
        errors.extend(function_errors);
    }
    
    Ok(errors)
}

pub fn check_borrows_with_annotations(program: IrProgram, header_cache: HeaderCache) -> Result<Vec<String>, String> {
    let mut errors = Vec::new();
    
    // Run regular borrow checking
    for function in &program.functions {
        let function_errors = check_function(function)?;
        errors.extend(function_errors);
    }
    
    // Run lifetime inference and validation
    for function in &program.functions {
        let inference_errors = lifetime_inference::infer_and_validate_lifetimes(function)?;
        errors.extend(inference_errors);
    }
    
    // If we have header annotations, also check lifetime constraints
    if header_cache.has_signatures() {
        let lifetime_errors = lifetime_checker::check_lifetimes_with_annotations(&program, &header_cache)?;
        errors.extend(lifetime_errors);
        
        // Also run scope-based lifetime checking
        let scope_errors = scope_lifetime::check_scoped_lifetimes(&program, &header_cache)?;
        errors.extend(scope_errors);
    }
    
    Ok(errors)
}

fn check_function(function: &IrFunction) -> Result<Vec<String>, String> {
    let mut errors = Vec::new();
    let mut ownership_tracker = OwnershipTracker::new();
    
    // Initialize ownership for parameters and variables
    for (name, var_info) in &function.variables {
        ownership_tracker.set_ownership(name.clone(), var_info.ownership.clone());
        
        // Track reference types
        match &var_info.ty {
            crate::ir::VariableType::Reference(_) => {
                ownership_tracker.mark_as_reference(name.clone(), false);
            }
            crate::ir::VariableType::MutableReference(_) => {
                ownership_tracker.mark_as_reference(name.clone(), true);
            }
            _ => {}
        }
    }
    
    // Traverse CFG and check each block
    for node_idx in function.cfg.node_indices() {
        let block = &function.cfg[node_idx];
        
        // Process statements, handling loops specially
        let mut i = 0;
        while i < block.statements.len() {
            let statement = &block.statements[i];
            
            // Check if we're entering a loop
            if matches!(statement, crate::ir::IrStatement::EnterLoop) {
                // Find the matching ExitLoop
                let mut loop_end = i + 1;
                let mut loop_depth = 1;
                while loop_end < block.statements.len() && loop_depth > 0 {
                    match &block.statements[loop_end] {
                        crate::ir::IrStatement::EnterLoop => loop_depth += 1,
                        crate::ir::IrStatement::ExitLoop => loop_depth -= 1,
                        _ => {}
                    }
                    loop_end += 1;
                }
                
                // Process the loop body twice to simulate 2 iterations
                let loop_body = &block.statements[i+1..loop_end-1];
                
                // First iteration
                ownership_tracker.enter_loop();
                
                // Track variables declared in the loop
                let mut loop_local_vars = HashSet::new();
                
                for loop_stmt in loop_body {
                    // Track variable declarations in the loop
                    if let crate::ir::IrStatement::Borrow { to, .. } = loop_stmt {
                        loop_local_vars.insert(to.clone());
                    }
                    process_statement(loop_stmt, &mut ownership_tracker, &mut errors);
                }
                
                // Save state after first iteration (but only for non-loop-local variables)
                let state_after_first = ownership_tracker.ownership.clone();
                
                // Clear loop-local borrows at end of first iteration
                ownership_tracker.clear_loop_locals(&loop_local_vars);
                
                // Second iteration - check for use-after-move
                for loop_stmt in loop_body {
                    // Before processing each statement in second iteration,
                    // check if it would cause use-after-move (but only for non-loop-local vars)
                    check_statement_for_loop_errors(loop_stmt, &state_after_first, &mut errors);
                    process_statement(loop_stmt, &mut ownership_tracker, &mut errors);
                }
                
                // Clear loop-local borrows at end of second iteration
                ownership_tracker.clear_loop_locals(&loop_local_vars);
                
                ownership_tracker.exit_loop();
                
                // Skip past the loop
                i = loop_end;
            } else {
                // Normal statement processing
                process_statement(statement, &mut ownership_tracker, &mut errors);
                i += 1;
            }
        }
    }
    
    Ok(errors)
}

// Helper function to check for loop-specific errors in second iteration
fn check_statement_for_loop_errors(
    statement: &crate::ir::IrStatement,
    state_after_first: &HashMap<String, OwnershipState>,
    errors: &mut Vec<String>,
) {
    match statement {
        crate::ir::IrStatement::Move { from, .. } => {
            if let Some(state) = state_after_first.get(from) {
                if *state == OwnershipState::Moved {
                    errors.push(format!(
                        "Use after move in loop: variable '{}' was moved in first iteration and used again in second iteration",
                        from
                    ));
                }
            }
        }
        crate::ir::IrStatement::Assign { rhs, .. } => {
            if let crate::ir::IrExpression::Variable(var) = rhs {
                if let Some(state) = state_after_first.get(var) {
                    if *state == OwnershipState::Moved {
                        errors.push(format!(
                            "Use after move in loop: variable '{}' was moved in first iteration and used again in second iteration",
                            var
                        ));
                    }
                }
            }
        }
        _ => {}
    }
}

// Extract statement processing logic into a separate function
fn process_statement(
    statement: &crate::ir::IrStatement,
    ownership_tracker: &mut OwnershipTracker,
    errors: &mut Vec<String>,
) {
    match statement {
        crate::ir::IrStatement::Move { from, to } => {
            // Check if 'from' is owned and not moved
            let from_state = ownership_tracker.get_ownership(from);
            
            // Can't move from a reference
            if ownership_tracker.is_reference(from) {
                errors.push(format!(
                    "Cannot move out of '{}' because it is behind a reference",
                    from
                ));
                return;
            }
            
            if from_state == Some(&OwnershipState::Moved) {
                errors.push(format!(
                    "Use after move: variable '{}' has already been moved",
                    from
                ));
            }
            
            // Handle temporary move markers (from std::move in function calls)
            if to.starts_with("_temp_move_") || to.starts_with("_moved_") {
                // Just mark the source as moved, don't create the temporary
                ownership_tracker.set_ownership(from.clone(), OwnershipState::Moved);
            } else {
                // Transfer ownership for regular moves
                ownership_tracker.set_ownership(from.clone(), OwnershipState::Moved);
                ownership_tracker.set_ownership(to.clone(), OwnershipState::Owned);
            }
        }
        
        crate::ir::IrStatement::Borrow { from, to, kind } => {
            // Check if the source is accessible
            let from_state = ownership_tracker.get_ownership(from);
            
            if from_state == Some(&OwnershipState::Moved) {
                errors.push(format!(
                    "Cannot borrow '{}' because it has been moved",
                    from
                ));
                return;
            }
            
            // Check existing borrows
            let current_borrows = ownership_tracker.get_borrows(from);
            
            match kind {
                BorrowKind::Immutable => {
                    // Can have multiple immutable borrows, but not if there's a mutable borrow
                    if current_borrows.has_mutable {
                        errors.push(format!(
                            "Cannot create immutable reference to '{}': already mutably borrowed",
                            from
                        ));
                    }
                    // In C++, const references are allowed even when the value is being modified
                    // through another path, but we enforce Rust's stricter rules
                }
                BorrowKind::Mutable => {
                    // Can only have one mutable borrow, and no immutable borrows
                    if current_borrows.immutable_count > 0 {
                        errors.push(format!(
                            "Cannot create mutable reference to '{}': already immutably borrowed",
                            from
                        ));
                    } else if current_borrows.has_mutable {
                        errors.push(format!(
                            "Cannot create mutable reference to '{}': already mutably borrowed",
                            from
                        ));
                    }
                }
            }
            
            // Record the borrow
            ownership_tracker.add_borrow(from.clone(), to.clone(), kind.clone());
            ownership_tracker.mark_as_reference(to.clone(), *kind == BorrowKind::Mutable);
        }
        
        crate::ir::IrStatement::Assign { lhs, rhs } => {
            // Check if we're trying to modify through a const reference
            if ownership_tracker.is_reference(lhs) && !ownership_tracker.is_mutable_reference(lhs) {
                errors.push(format!(
                    "Cannot assign to '{}' through const reference",
                    lhs
                ));
            }
            
            // Check if the rhs uses a moved variable
            if let crate::ir::IrExpression::Variable(rhs_var) = rhs {
                if ownership_tracker.get_ownership(rhs_var) == Some(&OwnershipState::Moved) {
                    errors.push(format!(
                        "Use after move: variable '{}' has been moved",
                        rhs_var
                    ));
                }
            }
        }
        
        crate::ir::IrStatement::EnterScope => {
            ownership_tracker.enter_scope();
        }
        
        crate::ir::IrStatement::ExitScope => {
            ownership_tracker.exit_scope();
        }
        
        crate::ir::IrStatement::EnterLoop => {
            // Handled at the higher level
        }
        
        crate::ir::IrStatement::ExitLoop => {
            // Handled at the higher level
        }
        
        _ => {}
    }
}

struct OwnershipTracker {
    ownership: HashMap<String, OwnershipState>,
    borrows: HashMap<String, BorrowInfo>,
    reference_info: HashMap<String, ReferenceInfo>,
    // Stack of scopes, each scope tracks borrows created in it
    scope_stack: Vec<ScopeInfo>,
    // Loop tracking
    loop_depth: usize,
    // Save state when entering a loop (for 2nd iteration checking)
    loop_entry_states: Vec<LoopEntryState>,
}

#[derive(Clone)]
struct LoopEntryState {
    ownership: HashMap<String, OwnershipState>,
    borrows: HashMap<String, BorrowInfo>,
}

#[derive(Default, Clone)]
struct ScopeInfo {
    // Borrows created in this scope (to be cleaned up on exit)
    local_borrows: HashSet<String>,
}

#[derive(Default, Clone)]
struct BorrowInfo {
    immutable_count: usize,
    has_mutable: bool,
    borrowers: HashSet<String>,
}

#[derive(Clone)]
struct ReferenceInfo {
    is_reference: bool,
    is_mutable: bool,
}

impl OwnershipTracker {
    fn new() -> Self {
        let mut tracker = Self {
            ownership: HashMap::new(),
            borrows: HashMap::new(),
            reference_info: HashMap::new(),
            scope_stack: Vec::new(),
            loop_depth: 0,
            loop_entry_states: Vec::new(),
        };
        // Start with a root scope
        tracker.scope_stack.push(ScopeInfo::default());
        tracker
    }
    
    fn set_ownership(&mut self, var: String, state: OwnershipState) {
        self.ownership.insert(var, state);
    }
    
    fn get_ownership(&self, var: &str) -> Option<&OwnershipState> {
        self.ownership.get(var)
    }
    
    fn get_borrows(&self, var: &str) -> BorrowInfo {
        self.borrows.get(var).cloned().unwrap_or_default()
    }
    
    fn add_borrow(&mut self, from: String, to: String, kind: BorrowKind) {
        let borrow_info = self.borrows.entry(from).or_default();
        borrow_info.borrowers.insert(to.clone());
        
        // Track this borrow in the current scope
        if let Some(current_scope) = self.scope_stack.last_mut() {
            current_scope.local_borrows.insert(to);
        }
        
        match kind {
            BorrowKind::Immutable => borrow_info.immutable_count += 1,
            BorrowKind::Mutable => borrow_info.has_mutable = true,
        }
    }
    
    fn enter_scope(&mut self) {
        self.scope_stack.push(ScopeInfo::default());
    }
    
    fn exit_scope(&mut self) {
        if let Some(scope) = self.scope_stack.pop() {
            // Clean up all borrows created in this scope
            for borrow_name in scope.local_borrows {
                // Remove from reference info
                self.reference_info.remove(&borrow_name);
                
                // Remove from all borrow tracking
                for borrow_info in self.borrows.values_mut() {
                    borrow_info.borrowers.remove(&borrow_name);
                    // Note: In a more complete implementation, we'd also
                    // decrement counts based on the borrow kind
                }
            }
            
            // Clean up empty borrow entries
            self.borrows.retain(|_, info| !info.borrowers.is_empty());
        }
    }
    
    fn mark_as_reference(&mut self, var: String, is_mutable: bool) {
        self.reference_info.insert(var, ReferenceInfo {
            is_reference: true,
            is_mutable,
        });
    }
    
    fn is_reference(&self, var: &str) -> bool {
        self.reference_info
            .get(var)
            .map(|info| info.is_reference)
            .unwrap_or(false)
    }
    
    fn is_mutable_reference(&self, var: &str) -> bool {
        self.reference_info
            .get(var)
            .map(|info| info.is_reference && info.is_mutable)
            .unwrap_or(false)
    }
    
    fn enter_loop(&mut self) {
        // Save current state when entering a loop
        // This state represents the state at the END of the first iteration
        // which is what we'll use to check the BEGINNING of the second iteration
        self.loop_entry_states.push(LoopEntryState {
            ownership: self.ownership.clone(),
            borrows: self.borrows.clone(),
        });
        self.loop_depth += 1;
    }
    
    fn exit_loop(&mut self) {
        if self.loop_depth > 0 {
            self.loop_depth -= 1;
            
            // When exiting a loop, we simulate having run it twice
            // The current state is after one iteration
            // We saved the state at loop entry, now apply the second iteration effects
            if let Some(entry_state) = self.loop_entry_states.pop() {
                // The key insight: variables that were moved in the loop body
                // will be moved at the START of the second iteration
                // So check if any variables that are currently Moved
                // were NOT moved at loop entry
                for (var, current_state) in &self.ownership {
                    if *current_state == OwnershipState::Moved {
                        // If this variable was Owned at loop entry,
                        // it means it was moved during the loop body
                        // On second iteration, it would already be Moved
                        if let Some(entry_ownership) = entry_state.ownership.get(var) {
                            if *entry_ownership == OwnershipState::Owned {
                                // Keep it as Moved - this correctly represents
                                // the state after 2 iterations
                                // The error will be caught if the variable is used
                                // in the loop body (which we already processed)
                            }
                        }
                    }
                }
            }
        }
    }
    
    fn clear_loop_locals(&mut self, loop_locals: &HashSet<String>) {
        // Clear borrows for loop-local variables
        for local_var in loop_locals {
            // Remove from reference info
            self.reference_info.remove(local_var);
            
            // Remove from all borrow tracking
            for borrow_info in self.borrows.values_mut() {
                borrow_info.borrowers.remove(local_var);
                // We should also decrement counts, but need to track the kind
                // For simplicity, we'll rebuild the counts
            }
            
            // Remove the ownership entry for loop-local variables
            self.ownership.remove(local_var);
        }
        
        // Clean up empty borrow entries and recalculate counts
        for (_, borrow_info) in self.borrows.iter_mut() {
            // Reset counts based on remaining borrowers
            // This is a simplification - in a real implementation we'd track
            // the kind of each borrow
            if borrow_info.borrowers.is_empty() {
                borrow_info.immutable_count = 0;
                borrow_info.has_mutable = false;
            }
        }
        
        // Remove empty entries
        self.borrows.retain(|_, info| !info.borrowers.is_empty());
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::ir::{IrProgram, IrFunction, BasicBlock, IrStatement};
    use petgraph::graph::DiGraph;

    fn create_test_program() -> IrProgram {
        IrProgram {
            functions: vec![],
            ownership_graph: DiGraph::new(),
        }
    }

    fn create_test_function(name: &str) -> IrFunction {
        let mut cfg = DiGraph::new();
        let block = BasicBlock {
            id: 0,
            statements: vec![],
            terminator: None,
        };
        cfg.add_node(block);
        
        IrFunction {
            name: name.to_string(),
            cfg,
            variables: HashMap::new(),
        }
    }

    #[test]
    fn test_empty_program_passes() {
        let program = create_test_program();
        let result = check_borrows(program);
        
        assert!(result.is_ok());
        let errors = result.unwrap();
        assert_eq!(errors.len(), 0);
    }

    #[test]
    fn test_ownership_tracker_initialization() {
        let mut tracker = OwnershipTracker::new();
        tracker.set_ownership("x".to_string(), OwnershipState::Owned);
        
        assert_eq!(tracker.get_ownership("x"), Some(&OwnershipState::Owned));
        assert_eq!(tracker.get_ownership("y"), None);
    }

    #[test]
    fn test_ownership_state_transitions() {
        let mut tracker = OwnershipTracker::new();
        
        // Start with owned
        tracker.set_ownership("x".to_string(), OwnershipState::Owned);
        assert_eq!(tracker.get_ownership("x"), Some(&OwnershipState::Owned));
        
        // Move to another variable
        tracker.set_ownership("x".to_string(), OwnershipState::Moved);
        tracker.set_ownership("y".to_string(), OwnershipState::Owned);
        
        assert_eq!(tracker.get_ownership("x"), Some(&OwnershipState::Moved));
        assert_eq!(tracker.get_ownership("y"), Some(&OwnershipState::Owned));
    }

    #[test]
    fn test_borrow_tracking() {
        let mut tracker = OwnershipTracker::new();
        tracker.set_ownership("x".to_string(), OwnershipState::Owned);
        
        // Add immutable borrow
        tracker.add_borrow("x".to_string(), "ref1".to_string(), BorrowKind::Immutable);
        let borrows = tracker.get_borrows("x");
        assert_eq!(borrows.immutable_count, 1);
        assert!(!borrows.has_mutable);
        
        // Add another immutable borrow
        tracker.add_borrow("x".to_string(), "ref2".to_string(), BorrowKind::Immutable);
        let borrows = tracker.get_borrows("x");
        assert_eq!(borrows.immutable_count, 2);
        assert!(!borrows.has_mutable);
    }

    #[test]
    fn test_mutable_borrow_tracking() {
        let mut tracker = OwnershipTracker::new();
        tracker.set_ownership("x".to_string(), OwnershipState::Owned);
        
        // Add mutable borrow
        tracker.add_borrow("x".to_string(), "mut_ref".to_string(), BorrowKind::Mutable);
        let borrows = tracker.get_borrows("x");
        assert_eq!(borrows.immutable_count, 0);
        assert!(borrows.has_mutable);
    }

    #[test]
    fn test_use_after_move_detection() {
        let mut program = create_test_program();
        let mut func = create_test_function("test");
        
        // Add variables
        func.variables.insert(
            "x".to_string(),
            crate::ir::VariableInfo {
                name: "x".to_string(),
                ty: crate::ir::VariableType::Owned("int".to_string()),
                ownership: OwnershipState::Owned,
                lifetime: None,
            },
        );
        
        func.variables.insert(
            "y".to_string(),
            crate::ir::VariableInfo {
                name: "y".to_string(),
                ty: crate::ir::VariableType::Owned("int".to_string()),
                ownership: OwnershipState::Owned,
                lifetime: None,
            },
        );
        
        // Add statements: move x to y, then try to use x
        let block = &mut func.cfg[petgraph::graph::NodeIndex::new(0)];
        block.statements.push(IrStatement::Move {
            from: "x".to_string(),
            to: "y".to_string(),
        });
        
        // Try to move x again (should fail)
        block.statements.push(IrStatement::Move {
            from: "x".to_string(),
            to: "z".to_string(),
        });
        
        program.functions.push(func);
        
        let result = check_borrows(program);
        assert!(result.is_ok());
        
        let errors = result.unwrap();
        assert_eq!(errors.len(), 1);
        assert!(errors[0].contains("Use after move"));
    }

    #[test]
    fn test_multiple_immutable_borrows_allowed() {
        let mut program = create_test_program();
        let mut func = create_test_function("test");
        
        func.variables.insert(
            "x".to_string(),
            crate::ir::VariableInfo {
                name: "x".to_string(),
                ty: crate::ir::VariableType::Owned("int".to_string()),
                ownership: OwnershipState::Owned,
                lifetime: None,
            },
        );
        
        let block = &mut func.cfg[petgraph::graph::NodeIndex::new(0)];
        block.statements.push(IrStatement::Borrow {
            from: "x".to_string(),
            to: "ref1".to_string(),
            kind: BorrowKind::Immutable,
        });
        
        block.statements.push(IrStatement::Borrow {
            from: "x".to_string(),
            to: "ref2".to_string(),
            kind: BorrowKind::Immutable,
        });
        
        program.functions.push(func);
        
        let result = check_borrows(program);
        assert!(result.is_ok());
        
        let errors = result.unwrap();
        assert_eq!(errors.len(), 0); // Multiple immutable borrows are OK
    }

    #[test]
    fn test_mutable_borrow_while_immutable_fails() {
        let mut program = create_test_program();
        let mut func = create_test_function("test");
        
        func.variables.insert(
            "x".to_string(),
            crate::ir::VariableInfo {
                name: "x".to_string(),
                ty: crate::ir::VariableType::Owned("int".to_string()),
                ownership: OwnershipState::Owned,
                lifetime: None,
            },
        );
        
        let block = &mut func.cfg[petgraph::graph::NodeIndex::new(0)];
        
        // First, immutable borrow
        block.statements.push(IrStatement::Borrow {
            from: "x".to_string(),
            to: "ref1".to_string(),
            kind: BorrowKind::Immutable,
        });
        
        // Then try mutable borrow (should fail)
        block.statements.push(IrStatement::Borrow {
            from: "x".to_string(),
            to: "mut_ref".to_string(),
            kind: BorrowKind::Mutable,
        });
        
        program.functions.push(func);
        
        let result = check_borrows(program);
        assert!(result.is_ok());
        
        let errors = result.unwrap();
        assert_eq!(errors.len(), 1);
        assert!(errors[0].contains("Cannot"));
        assert!(errors[0].contains("mutable"));
    }

    #[test]
    fn test_const_reference_cannot_modify() {
        let mut program = create_test_program();
        let mut func = create_test_function("test");
        
        // Add a value and a const reference to it
        func.variables.insert(
            "value".to_string(),
            crate::ir::VariableInfo {
                name: "value".to_string(),
                ty: crate::ir::VariableType::Owned("int".to_string()),
                ownership: OwnershipState::Owned,
                lifetime: None,
            },
        );
        
        func.variables.insert(
            "const_ref".to_string(),
            crate::ir::VariableInfo {
                name: "const_ref".to_string(),
                ty: crate::ir::VariableType::Reference("int".to_string()),
                ownership: OwnershipState::Borrowed(BorrowKind::Immutable),
                lifetime: None,
            },
        );
        
        let block = &mut func.cfg[petgraph::graph::NodeIndex::new(0)];
        
        // Create const reference
        block.statements.push(IrStatement::Borrow {
            from: "value".to_string(),
            to: "const_ref".to_string(),
            kind: BorrowKind::Immutable,
        });
        
        // Try to modify through const reference (should fail)
        block.statements.push(IrStatement::Assign {
            lhs: "const_ref".to_string(),
            rhs: crate::ir::IrExpression::Variable("other".to_string()),
        });
        
        program.functions.push(func);
        
        let result = check_borrows(program);
        assert!(result.is_ok());
        
        let errors = result.unwrap();
        assert_eq!(errors.len(), 1);
        assert!(errors[0].contains("Cannot assign"));
        assert!(errors[0].contains("const reference"));
    }

    #[test]
    fn test_mutable_reference_can_modify() {
        let mut program = create_test_program();
        let mut func = create_test_function("test");
        
        // Add a value and a mutable reference to it
        func.variables.insert(
            "value".to_string(),
            crate::ir::VariableInfo {
                name: "value".to_string(),
                ty: crate::ir::VariableType::Owned("int".to_string()),
                ownership: OwnershipState::Owned,
                lifetime: None,
            },
        );
        
        func.variables.insert(
            "mut_ref".to_string(),
            crate::ir::VariableInfo {
                name: "mut_ref".to_string(),
                ty: crate::ir::VariableType::MutableReference("int".to_string()),
                ownership: OwnershipState::Borrowed(BorrowKind::Mutable),
                lifetime: None,
            },
        );
        
        let block = &mut func.cfg[petgraph::graph::NodeIndex::new(0)];
        
        // Create mutable reference
        block.statements.push(IrStatement::Borrow {
            from: "value".to_string(),
            to: "mut_ref".to_string(),
            kind: BorrowKind::Mutable,
        });
        
        // Modify through mutable reference (should succeed)
        block.statements.push(IrStatement::Assign {
            lhs: "mut_ref".to_string(),
            rhs: crate::ir::IrExpression::Variable("other".to_string()),
        });
        
        program.functions.push(func);
        
        let result = check_borrows(program);
        assert!(result.is_ok());
        
        let errors = result.unwrap();
        assert_eq!(errors.len(), 0); // Should succeed
    }

    #[test]
    fn test_cannot_move_from_reference() {
        let mut program = create_test_program();
        let mut func = create_test_function("test");
        
        // Add a reference variable
        func.variables.insert(
            "ref_var".to_string(),
            crate::ir::VariableInfo {
                name: "ref_var".to_string(),
                ty: crate::ir::VariableType::Reference("int".to_string()),
                ownership: OwnershipState::Borrowed(BorrowKind::Immutable),
                lifetime: None,
            },
        );
        
        func.variables.insert(
            "dest".to_string(),
            crate::ir::VariableInfo {
                name: "dest".to_string(),
                ty: crate::ir::VariableType::Owned("int".to_string()),
                ownership: OwnershipState::Owned,
                lifetime: None,
            },
        );
        
        let block = &mut func.cfg[petgraph::graph::NodeIndex::new(0)];
        
        // Try to move from reference (should fail)
        block.statements.push(IrStatement::Move {
            from: "ref_var".to_string(),
            to: "dest".to_string(),
        });
        
        program.functions.push(func);
        
        let result = check_borrows(program);
        assert!(result.is_ok());
        
        let errors = result.unwrap();
        assert_eq!(errors.len(), 1);
        assert!(errors[0].contains("Cannot move"));
        assert!(errors[0].contains("reference"));
    }

    #[test]
    fn test_multiple_const_references_allowed() {
        let mut program = create_test_program();
        let mut func = create_test_function("test");
        
        func.variables.insert(
            "value".to_string(),
            crate::ir::VariableInfo {
                name: "value".to_string(),
                ty: crate::ir::VariableType::Owned("int".to_string()),
                ownership: OwnershipState::Owned,
                lifetime: None,
            },
        );
        
        let block = &mut func.cfg[petgraph::graph::NodeIndex::new(0)];
        
        // Create multiple const references (should succeed)
        block.statements.push(IrStatement::Borrow {
            from: "value".to_string(),
            to: "const_ref1".to_string(),
            kind: BorrowKind::Immutable,
        });
        
        block.statements.push(IrStatement::Borrow {
            from: "value".to_string(),
            to: "const_ref2".to_string(),
            kind: BorrowKind::Immutable,
        });
        
        block.statements.push(IrStatement::Borrow {
            from: "value".to_string(),
            to: "const_ref3".to_string(),
            kind: BorrowKind::Immutable,
        });
        
        program.functions.push(func);
        
        let result = check_borrows(program);
        assert!(result.is_ok());
        
        let errors = result.unwrap();
        assert_eq!(errors.len(), 0); // Multiple const references are allowed
    }

    #[test]
    fn test_cannot_borrow_moved_value() {
        let mut program = create_test_program();
        let mut func = create_test_function("test");
        
        func.variables.insert(
            "value".to_string(),
            crate::ir::VariableInfo {
                name: "value".to_string(),
                ty: crate::ir::VariableType::UniquePtr("int".to_string()),
                ownership: OwnershipState::Owned,
                lifetime: None,
            },
        );
        
        let block = &mut func.cfg[petgraph::graph::NodeIndex::new(0)];
        
        // Move the value
        block.statements.push(IrStatement::Move {
            from: "value".to_string(),
            to: "other".to_string(),
        });
        
        // Try to create reference to moved value (should fail)
        block.statements.push(IrStatement::Borrow {
            from: "value".to_string(),
            to: "ref".to_string(),
            kind: BorrowKind::Immutable,
        });
        
        program.functions.push(func);
        
        let result = check_borrows(program);
        assert!(result.is_ok());
        
        let errors = result.unwrap();
        assert!(errors.len() > 0);
        assert!(errors.iter().any(|e| e.contains("moved")));
    }

    #[test]
    fn test_multiple_functions_with_references() {
        let mut program = create_test_program();
        
        // First function with valid const refs
        let mut func1 = create_test_function("func1");
        func1.variables.insert(
            "x".to_string(),
            crate::ir::VariableInfo {
                name: "x".to_string(),
                ty: crate::ir::VariableType::Owned("int".to_string()),
                ownership: OwnershipState::Owned,
                lifetime: None,
            },
        );
        
        let block1 = &mut func1.cfg[petgraph::graph::NodeIndex::new(0)];
        block1.statements.push(IrStatement::Borrow {
            from: "x".to_string(),
            to: "ref1".to_string(),
            kind: BorrowKind::Immutable,
        });
        block1.statements.push(IrStatement::Borrow {
            from: "x".to_string(),
            to: "ref2".to_string(),
            kind: BorrowKind::Immutable,
        });
        
        // Second function with invalid refs
        let mut func2 = create_test_function("func2");
        func2.variables.insert(
            "y".to_string(),
            crate::ir::VariableInfo {
                name: "y".to_string(),
                ty: crate::ir::VariableType::Owned("int".to_string()),
                ownership: OwnershipState::Owned,
                lifetime: None,
            },
        );
        
        let block2 = &mut func2.cfg[petgraph::graph::NodeIndex::new(0)];
        block2.statements.push(IrStatement::Borrow {
            from: "y".to_string(),
            to: "mut1".to_string(),
            kind: BorrowKind::Mutable,
        });
        block2.statements.push(IrStatement::Borrow {
            from: "y".to_string(),
            to: "mut2".to_string(),
            kind: BorrowKind::Mutable,
        });
        
        program.functions.push(func1);
        program.functions.push(func2);
        
        let result = check_borrows(program);
        assert!(result.is_ok());
        
        let errors = result.unwrap();
        assert_eq!(errors.len(), 1); // Only func2 should have errors
        assert!(errors[0].contains("already mutably borrowed"));
    }

    #[test]
    fn test_complex_borrow_chain() {
        let mut program = create_test_program();
        let mut func = create_test_function("test");
        
        // Create variables
        func.variables.insert(
            "a".to_string(),
            crate::ir::VariableInfo {
                name: "a".to_string(),
                ty: crate::ir::VariableType::Owned("int".to_string()),
                ownership: OwnershipState::Owned,
                lifetime: None,
            },
        );
        
        func.variables.insert(
            "b".to_string(),
            crate::ir::VariableInfo {
                name: "b".to_string(),
                ty: crate::ir::VariableType::Owned("int".to_string()),
                ownership: OwnershipState::Owned,
                lifetime: None,
            },
        );
        
        let block = &mut func.cfg[petgraph::graph::NodeIndex::new(0)];
        
        // Create multiple immutable refs to 'a'
        block.statements.push(IrStatement::Borrow {
            from: "a".to_string(),
            to: "ref_a1".to_string(),
            kind: BorrowKind::Immutable,
        });
        block.statements.push(IrStatement::Borrow {
            from: "a".to_string(),
            to: "ref_a2".to_string(),
            kind: BorrowKind::Immutable,
        });
        
        // Create mutable ref to 'b'
        block.statements.push(IrStatement::Borrow {
            from: "b".to_string(),
            to: "mut_b".to_string(),
            kind: BorrowKind::Mutable,
        });
        
        // Try to create another ref to 'b' (should fail)
        block.statements.push(IrStatement::Borrow {
            from: "b".to_string(),
            to: "ref_b".to_string(),
            kind: BorrowKind::Immutable,
        });
        
        program.functions.push(func);
        
        let result = check_borrows(program);
        assert!(result.is_ok());
        
        let errors = result.unwrap();
        assert_eq!(errors.len(), 1);
        assert!(errors[0].contains("'b'"));
        assert!(errors[0].contains("already mutably borrowed"));
    }
}
#[cfg(test)]
mod scope_tests {
    use super::*;
    use crate::ir::{BasicBlock, IrFunction, IrProgram, IrStatement, BorrowKind};
    use petgraph::graph::Graph;
    use std::collections::HashMap;

    fn create_test_function_with_statements(statements: Vec<IrStatement>) -> IrFunction {
        let mut cfg = Graph::new();
        let block = BasicBlock {
            id: 0,
            statements,
            terminator: None,
        };
        cfg.add_node(block);
        
        IrFunction {
            name: "test".to_string(),
            cfg,
            variables: HashMap::new(),
        }
    }

    #[test]
    fn test_scope_cleanup_simple() {
        let statements = vec![
            IrStatement::EnterScope,
            IrStatement::Borrow {
                from: "value".to_string(),
                to: "ref1".to_string(),
                kind: BorrowKind::Mutable,
            },
            IrStatement::ExitScope,
            // After scope exit, should be able to borrow again
            IrStatement::Borrow {
                from: "value".to_string(),
                to: "ref2".to_string(),
                kind: BorrowKind::Mutable,
            },
        ];
        
        let func = create_test_function_with_statements(statements);
        let mut program = IrProgram {
            functions: vec![func],
            ownership_graph: petgraph::graph::DiGraph::new(),
        };
        
        let result = check_borrows(program);
        assert!(result.is_ok());
        let errors = result.unwrap();
        assert_eq!(errors.len(), 0, "Should not report errors for borrows in different scopes");
    }

    #[test]
    fn test_nested_scopes() {
        let statements = vec![
            IrStatement::EnterScope,
            IrStatement::Borrow {
                from: "value".to_string(),
                to: "ref1".to_string(),
                kind: BorrowKind::Immutable,
            },
            IrStatement::EnterScope,
            // Nested scope - should be able to have another immutable borrow
            IrStatement::Borrow {
                from: "value".to_string(),
                to: "ref2".to_string(),
                kind: BorrowKind::Immutable,
            },
            IrStatement::ExitScope,
            // ref2 is gone, but ref1 still exists
            IrStatement::ExitScope,
            // Now both are gone
            IrStatement::Borrow {
                from: "value".to_string(),
                to: "ref3".to_string(),
                kind: BorrowKind::Mutable,
            },
        ];
        
        let func = create_test_function_with_statements(statements);
        let mut program = IrProgram {
            functions: vec![func],
            ownership_graph: petgraph::graph::DiGraph::new(),
        };
        
        let result = check_borrows(program);
        assert!(result.is_ok());
        let errors = result.unwrap();
        assert_eq!(errors.len(), 0, "Nested scopes should work correctly");
    }

    #[test]
    fn test_scope_doesnt_affect_moves() {
        let statements = vec![
            IrStatement::EnterScope,
            IrStatement::Move {
                from: "x".to_string(),
                to: "y".to_string(),
            },
            IrStatement::ExitScope,
            // x is still moved even after scope exit
            IrStatement::Move {
                from: "x".to_string(),
                to: "z".to_string(),
            },
        ];
        
        let func = create_test_function_with_statements(statements);
        let mut program = IrProgram {
            functions: vec![func],
            ownership_graph: petgraph::graph::DiGraph::new(),
        };
        
        let result = check_borrows(program);
        assert!(result.is_ok());
        let errors = result.unwrap();
        assert!(errors.len() > 0, "Should still detect use-after-move across scopes");
        assert!(errors[0].contains("already been moved") || errors[0].contains("Use after move"));
    }

    #[test]
    fn test_multiple_sequential_scopes() {
        let statements = vec![
            // First scope
            IrStatement::EnterScope,
            IrStatement::Borrow {
                from: "value".to_string(),
                to: "ref1".to_string(),
                kind: BorrowKind::Mutable,
            },
            IrStatement::ExitScope,
            
            // Second scope
            IrStatement::EnterScope,
            IrStatement::Borrow {
                from: "value".to_string(),
                to: "ref2".to_string(),
                kind: BorrowKind::Mutable,
            },
            IrStatement::ExitScope,
            
            // Third scope
            IrStatement::EnterScope,
            IrStatement::Borrow {
                from: "value".to_string(),
                to: "ref3".to_string(),
                kind: BorrowKind::Mutable,
            },
            IrStatement::ExitScope,
        ];
        
        let func = create_test_function_with_statements(statements);
        let mut program = IrProgram {
            functions: vec![func],
            ownership_graph: petgraph::graph::DiGraph::new(),
        };
        
        let result = check_borrows(program);
        assert!(result.is_ok());
        let errors = result.unwrap();
        assert_eq!(errors.len(), 0, "Sequential scopes should not conflict");
    }

    #[test]
    fn test_error_still_caught_in_same_scope() {
        let statements = vec![
            IrStatement::EnterScope,
            IrStatement::Borrow {
                from: "value".to_string(),
                to: "ref1".to_string(),
                kind: BorrowKind::Mutable,
            },
            // This should error - same scope
            IrStatement::Borrow {
                from: "value".to_string(),
                to: "ref2".to_string(),
                kind: BorrowKind::Mutable,
            },
            IrStatement::ExitScope,
        ];
        
        let func = create_test_function_with_statements(statements);
        let mut program = IrProgram {
            functions: vec![func],
            ownership_graph: petgraph::graph::DiGraph::new(),
        };
        
        let result = check_borrows(program);
        assert!(result.is_ok());
        let errors = result.unwrap();
        assert!(errors.len() > 0, "Should still catch errors within the same scope");
        assert!(errors[0].contains("already mutably borrowed"));
    }
}
