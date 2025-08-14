use std::path::Path;
use std::fs::File;
use std::io::{BufRead, BufReader};
use clang::Entity;

#[derive(Debug, Clone, PartialEq)]
pub enum SafetyMode {
    Safe,    // Enforce borrow checking
    Unsafe,  // Skip borrow checking
    Default, // Use parent context or default (unsafe)
}

#[derive(Debug, Clone)]
pub struct SafetyContext {
    pub file_default: SafetyMode,
    pub function_overrides: Vec<(String, SafetyMode)>, // Function name -> safety mode
    pub block_regions: Vec<UnsafeRegion>, // Line-based unsafe regions within safe functions
}

#[derive(Debug, Clone)]
pub struct UnsafeRegion {
    pub start_line: usize,
    pub end_line: usize,
}

impl SafetyContext {
    pub fn new() -> Self {
        Self {
            file_default: SafetyMode::Default,
            function_overrides: Vec::new(),
            block_regions: Vec::new(),
        }
    }
    
    /// Check if a specific function should be checked
    pub fn should_check_function(&self, func_name: &str) -> bool {
        // First check for function-specific override
        for (name, mode) in &self.function_overrides {
            if name == func_name {
                return *mode == SafetyMode::Safe;
            }
        }
        
        // Fall back to file default
        self.file_default == SafetyMode::Safe
    }
    
    /// Check if a line is in an unsafe region
    pub fn is_line_unsafe(&self, line: usize) -> bool {
        self.block_regions.iter().any(|r| line >= r.start_line && line <= r.end_line)
    }
}

/// Parse safety annotations from a C++ file using the unified rule:
/// @safe/@unsafe attaches to the next statement/block/function/namespace
pub fn parse_safety_annotations(path: &Path) -> Result<SafetyContext, String> {
    let file = File::open(path)
        .map_err(|e| format!("Failed to open file for safety parsing: {}", e))?;
    
    let reader = BufReader::new(file);
    let mut context = SafetyContext::new();
    let mut pending_annotation: Option<SafetyMode> = None;
    let mut in_comment_block = false;
    let mut current_line = 0;
    
    for line_result in reader.lines() {
        current_line += 1;
        let line = line_result.map_err(|e| format!("Failed to read line: {}", e))?;
        let trimmed = line.trim();
        
        // Handle multi-line comments
        if in_comment_block {
            if trimmed.contains("*/") {
                in_comment_block = false;
            }
            // Check for annotations in multi-line comments
            if trimmed.contains("@safe") {
                pending_annotation = Some(SafetyMode::Safe);
            } else if trimmed.contains("@unsafe") {
                pending_annotation = Some(SafetyMode::Unsafe);
            }
            continue;
        }
        
        // Check for comment start
        if trimmed.starts_with("/*") {
            in_comment_block = true;
            if trimmed.contains("@safe") {
                pending_annotation = Some(SafetyMode::Safe);
            } else if trimmed.contains("@unsafe") {
                pending_annotation = Some(SafetyMode::Unsafe);
            }
            continue;
        }
        
        // Check single-line comments
        if trimmed.starts_with("//") {
            if trimmed.contains("@safe") {
                pending_annotation = Some(SafetyMode::Safe);
            } else if trimmed.contains("@unsafe") {
                pending_annotation = Some(SafetyMode::Unsafe);
            }
            continue;
        }
        
        // Skip empty lines and preprocessor directives
        if trimmed.is_empty() || trimmed.starts_with("#") {
            continue;
        }
        
        // If we have a pending annotation and found code, apply it
        if let Some(annotation) = pending_annotation.take() {
            // Check what kind of code element follows
            if trimmed.starts_with("namespace") || 
               (trimmed.contains("namespace") && !trimmed.contains("using")) {
                // Namespace declaration - applies to whole file/namespace
                context.file_default = annotation;
            } else if is_function_declaration(trimmed) {
                // Function declaration - extract function name and apply
                if let Some(func_name) = extract_function_name(trimmed) {
                    context.function_overrides.push((func_name, annotation));
                }
            } else if trimmed == "{" || trimmed.ends_with("{") {
                // Block start - this would be an unsafe block within safe code
                // For now, we'll handle this through the separate unsafe region scanner
                // This is a placeholder for future enhancement
            } else {
                // Any other code - if this is the first code in file, apply as file default
                if context.file_default == SafetyMode::Default {
                    context.file_default = annotation;
                }
            }
        }
    }
    
    // Also scan for inline unsafe regions (// @unsafe ... // @endunsafe)
    context.block_regions = scan_unsafe_regions(path)?;
    
    Ok(context)
}

/// Scan for unsafe block regions marked with // @unsafe and // @endunsafe
fn scan_unsafe_regions(path: &Path) -> Result<Vec<UnsafeRegion>, String> {
    let file = File::open(path)
        .map_err(|e| format!("Failed to open file for unsafe scanning: {}", e))?;
    
    let reader = BufReader::new(file);
    let mut regions = Vec::new();
    let mut unsafe_stack: Vec<usize> = Vec::new();
    
    for (line_num, line_result) in reader.lines().enumerate() {
        let line = line_result.map_err(|e| format!("Failed to read line: {}", e))?;
        let trimmed = line.trim();
        
        // Check for unsafe start marker
        if (trimmed.contains("// @unsafe") || trimmed.contains("//@unsafe")) && 
           !trimmed.contains("@endunsafe") {
            unsafe_stack.push(line_num + 1); // Convert to 1-based line numbers
        }
        // Check for unsafe end marker
        else if trimmed.contains("// @endunsafe") || trimmed.contains("//@endunsafe") {
            if let Some(start) = unsafe_stack.pop() {
                regions.push(UnsafeRegion {
                    start_line: start,
                    end_line: line_num + 1,
                });
            }
        }
    }
    
    Ok(regions)
}

/// Check if a line looks like a function declaration
fn is_function_declaration(line: &str) -> bool {
    // Simple heuristic - contains parentheses and common return types
    // This is simplified and could be improved
    let has_parens = line.contains('(') && line.contains(')');
    let has_type = line.contains("void") || line.contains("int") || 
                   line.contains("bool") || line.contains("auto") ||
                   line.contains("const") || line.contains("static");
    
    has_parens && (has_type || line.contains("::"))
}

/// Extract function name from a declaration line
fn extract_function_name(line: &str) -> Option<String> {
    // Find the function name before the opening parenthesis
    if let Some(paren_pos) = line.find('(') {
        let before_paren = &line[..paren_pos];
        // Split by whitespace and get the last identifier
        let parts: Vec<&str> = before_paren.split_whitespace().collect();
        if let Some(last) = parts.last() {
            // Remove any qualifiers like * or &
            let name = last.trim_start_matches('*').trim_start_matches('&');
            if !name.is_empty() {
                return Some(name.to_string());
            }
        }
    }
    None
}

/// Parse safety annotation from entity comment (for clang AST)
pub fn parse_entity_safety(entity: &Entity) -> Option<SafetyMode> {
    if let Some(comment) = entity.get_comment() {
        if comment.contains("@safe") {
            Some(SafetyMode::Safe)
        } else if comment.contains("@unsafe") {
            Some(SafetyMode::Unsafe)
        } else {
            None
        }
    } else {
        None
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use tempfile::NamedTempFile;
    use std::io::Write;
    
    #[test]
    fn test_namespace_safe_annotation() {
        let code = r#"
// @safe
namespace myapp {
    void func1() {}
    void func2() {}
}
"#;
        
        let mut file = NamedTempFile::with_suffix(".cpp").unwrap();
        file.write_all(code.as_bytes()).unwrap();
        file.flush().unwrap();
        
        let context = parse_safety_annotations(file.path()).unwrap();
        assert_eq!(context.file_default, SafetyMode::Safe);
    }
    
    #[test]
    fn test_function_safe_annotation() {
        let code = r#"
// Default is unsafe
void unsafe_func() {}

// @safe
void safe_func() {
    int x = 42;
}

// @unsafe
void explicit_unsafe() {}
"#;
        
        let mut file = NamedTempFile::with_suffix(".cpp").unwrap();
        file.write_all(code.as_bytes()).unwrap();
        file.flush().unwrap();
        
        let context = parse_safety_annotations(file.path()).unwrap();
        
        assert!(!context.should_check_function("unsafe_func"));
        assert!(context.should_check_function("safe_func"));
        assert!(!context.should_check_function("explicit_unsafe"));
    }
    
    #[test]
    fn test_first_code_element_annotation() {
        let code = r#"
// @safe
int global_var = 42;

void func() {}
"#;
        
        let mut file = NamedTempFile::with_suffix(".cpp").unwrap();
        file.write_all(code.as_bytes()).unwrap();
        file.flush().unwrap();
        
        let context = parse_safety_annotations(file.path()).unwrap();
        assert_eq!(context.file_default, SafetyMode::Safe);
    }
    
    #[test]
    fn test_unsafe_regions() {
        let code = r#"
// @safe
void func() {
    int x = 42;
    // @unsafe
    int& ref1 = x;
    int& ref2 = x;
    // @endunsafe
    int& ref3 = x;
}
"#;
        
        let mut file = NamedTempFile::with_suffix(".cpp").unwrap();
        file.write_all(code.as_bytes()).unwrap();
        file.flush().unwrap();
        
        let context = parse_safety_annotations(file.path()).unwrap();
        
        assert!(!context.is_line_unsafe(4)); // int x = 42;
        assert!(context.is_line_unsafe(6));  // int& ref1 = x; (in unsafe region)
        assert!(context.is_line_unsafe(7));  // int& ref2 = x; (in unsafe region)
        assert!(!context.is_line_unsafe(9)); // int& ref3 = x; (outside unsafe)
    }
}