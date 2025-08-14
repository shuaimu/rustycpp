use clang::{Clang, Entity, EntityKind, Index};
use std::path::Path;

pub mod ast_visitor;
pub mod annotations;
pub mod header_cache;
pub mod unsafe_scanner;
pub mod safety_annotations;

pub use ast_visitor::{CppAst, Function, Statement, Expression};
pub use header_cache::HeaderCache;
#[allow(unused_imports)]
pub use ast_visitor::{Variable, SourceLocation};

use std::fs;
use std::io::{BufRead, BufReader};

pub fn parse_cpp_file(path: &Path) -> Result<CppAst, String> {
    parse_cpp_file_with_includes(path, &[])
}

pub fn parse_cpp_file_with_includes(path: &Path, include_paths: &[std::path::PathBuf]) -> Result<CppAst, String> {
    // Initialize Clang
    let clang = Clang::new()
        .map_err(|e| format!("Failed to initialize Clang: {:?}", e))?;
    
    let index = Index::new(&clang, false, false);
    
    // Build arguments with include paths
    let mut args = vec!["-std=c++17".to_string(), "-xc++".to_string()];
    for include_path in include_paths {
        args.push(format!("-I{}", include_path.display()));
    }
    
    // Parse the translation unit
    let tu = index
        .parser(path)
        .arguments(&args.iter().map(|s| s.as_str()).collect::<Vec<_>>())
        .parse()
        .map_err(|e| format!("Failed to parse file: {:?}", e))?;
    
    // Check for diagnostics
    let diagnostics = tu.get_diagnostics();
    if !diagnostics.is_empty() {
        for diag in &diagnostics {
            if diag.get_severity() >= clang::diagnostic::Severity::Error {
                return Err(format!("Parse error: {}", diag.get_text()));
            }
        }
    }
    
    // Visit the AST
    let mut ast = CppAst::new();
    let root = tu.get_entity();
    visit_entity(&root, &mut ast);
    
    Ok(ast)
}

fn visit_entity(entity: &Entity, ast: &mut CppAst) {
    match entity.get_kind() {
        EntityKind::FunctionDecl | EntityKind::Method => {
            if entity.is_definition() {
                let func = ast_visitor::extract_function(entity);
                ast.functions.push(func);
            }
        }
        EntityKind::VarDecl => {
            let var = ast_visitor::extract_variable(entity);
            ast.global_variables.push(var);
        }
        _ => {}
    }
    
    // Recursively visit children
    for child in entity.get_children() {
        visit_entity(&child, ast);
    }
}

/// Check if the file has @safe annotation at the beginning
pub fn check_file_safety_annotation(path: &Path) -> Result<bool, String> {
    let file = fs::File::open(path)
        .map_err(|e| format!("Failed to open file for safety check: {}", e))?;
    
    let reader = BufReader::new(file);
    
    // Check first 20 lines for @safe annotation (before any code)
    let mut found_code = false;
    for (line_num, line_result) in reader.lines().enumerate() {
        if line_num > 20 {
            break; // Don't look too far
        }
        
        let line = line_result.map_err(|e| format!("Failed to read line: {}", e))?;
        let trimmed = line.trim();
        
        // Skip empty lines
        if trimmed.is_empty() {
            continue;
        }
        
        // Check for @safe annotation in comments
        if trimmed.starts_with("//") {
            if trimmed.contains("@safe") {
                return Ok(true);
            }
        } else if trimmed.starts_with("/*") {
            // Check multi-line comment for @safe
            if line.contains("@safe") {
                return Ok(true);
            }
        } else if !trimmed.starts_with("#") {
            // Found actual code (not preprocessor), stop looking
            found_code = true;
            break;
        }
    }
    
    Ok(false) // No @safe annotation found
}

#[cfg(test)]
mod tests {
    use super::*;
    use tempfile::NamedTempFile;
    use std::io::Write;

    fn create_temp_cpp_file(content: &str) -> NamedTempFile {
        let mut file = NamedTempFile::with_suffix(".cpp").unwrap();
        file.write_all(content.as_bytes()).unwrap();
        file.flush().unwrap();
        file
    }
    
    #[allow(dead_code)]
    fn is_libclang_available() -> bool {
        // Try to initialize Clang to check if libclang is available
        // Note: This might fail if another test already initialized Clang
        true  // Assume it's available and let individual tests handle errors
    }

    #[test]
    fn test_parse_simple_function() {
        let code = r#"
        void test_function() {
            int x = 42;
        }
        "#;
        
        let temp_file = create_temp_cpp_file(code);
        let result = parse_cpp_file(temp_file.path());
        
        match result {
            Ok(ast) => {
                assert_eq!(ast.functions.len(), 1);
                assert_eq!(ast.functions[0].name, "test_function");
            }
            Err(e) if e.contains("already exists") => {
                // Skip if Clang is already initialized by another test
                eprintln!("Skipping test: Clang already initialized by another test");
            }
            Err(e) if e.contains("Failed to initialize Clang") => {
                // Skip if libclang is not available
                eprintln!("Skipping test: libclang not available");
            }
            Err(e) => {
                panic!("Unexpected error: {}", e);
            }
        }
    }

    #[test]
    fn test_parse_function_with_parameters() {
        let code = r#"
        int add(int a, int b) {
            return a + b;
        }
        "#;
        
        let temp_file = create_temp_cpp_file(code);
        let result = parse_cpp_file(temp_file.path());
        
        match result {
            Ok(ast) => {
                assert_eq!(ast.functions.len(), 1);
                assert_eq!(ast.functions[0].name, "add");
                assert_eq!(ast.functions[0].parameters.len(), 2);
            }
            Err(e) if e.contains("already exists") => {
                eprintln!("Skipping test: Clang already initialized by another test");
            }
            Err(e) if e.contains("Failed to initialize Clang") => {
                eprintln!("Skipping test: libclang not available");
            }
            Err(e) => {
                panic!("Unexpected error: {}", e);
            }
        }
    }

    #[test]
    fn test_parse_global_variable() {
        let code = r#"
        int global_var = 100;
        
        void func() {}
        "#;
        
        let temp_file = create_temp_cpp_file(code);
        let result = parse_cpp_file(temp_file.path());
        
        match result {
            Ok(ast) => {
                assert_eq!(ast.global_variables.len(), 1);
                assert_eq!(ast.global_variables[0].name, "global_var");
            }
            Err(e) if e.contains("already exists") => {
                eprintln!("Skipping test: Clang already initialized by another test");
            }
            Err(e) if e.contains("Failed to initialize Clang") => {
                eprintln!("Skipping test: libclang not available");
            }
            Err(e) => {
                panic!("Unexpected error: {}", e);
            }
        }
    }

    #[test]
    fn test_parse_invalid_file() {
        let temp_dir = tempfile::tempdir().unwrap();
        let invalid_path = temp_dir.path().join("nonexistent.cpp");
        
        let result = parse_cpp_file(&invalid_path);
        assert!(result.is_err());
    }
}